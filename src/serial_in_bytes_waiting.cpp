#include "cpp_core/error_callback.h"
#include "cpp_core/status_codes.h"
#include "serial_internal.hpp"

#include <asm-generic/ioctls.h>
#include <cpp_core/interface/serial_in_bytes_waiting.h>
#include <cstdint>
#include <mutex>
#include <sys/ioctl.h>
#include <utility>

using namespace serial_internal;

extern "C" auto serialInBytesWaiting(int64_t handle_ptr, ErrorCallbackT error_callback) -> int
{
    auto *handle = reinterpret_cast<SerialPortHandle *>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError),
            "serialInBytesWaiting: Invalid handle",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    std::scoped_lock const guard(handle->mtx);

    int available = 0;
    if (ioctl(handle->fd, FIONREAD, &available) == -1)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kGetStateError),
            "serialInBytesWaiting: ioctl failed",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kGetStateError);
    }
    return available;
}
