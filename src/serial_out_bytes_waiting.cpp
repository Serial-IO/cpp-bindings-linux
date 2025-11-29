#include "cpp_core/error_callback.h"
#include "cpp_core/status_codes.h"
#include "serial_internal.hpp"

#include <cpp_core/interface/serial_out_bytes_waiting.h>
#include <cstdint>
#include <mutex>
#include <sys/ioctl.h>
#include <utility>

using namespace serial_internal;

extern "C" auto serialOutBytesWaiting(int64_t handle_ptr, ErrorCallbackT error_callback) -> int
{
    auto *handle = reinterpret_cast<SerialPortHandle *>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError),
            "serialOutBytesWaiting: Invalid handle",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    std::scoped_lock const guard(handle->mtx);

    int queued = 0;
#ifdef TIOCOUTQ
    if (ioctl(handle->fd, TIOCOUTQ, &queued) == -1)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kGetStateError),
            "serialOutBytesWaiting: ioctl failed",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kGetStateError);
    }
#else
    queued = 0;
#endif
    return queued;
}
