#include "cpp_core/error_callback.h"
#include "cpp_core/status_codes.h"
#include "serial_internal.hpp"

#include <cpp_core/interface/serial_drain.h>
#include <cstdint>
#include <mutex>
#include <termios.h>
#include <utility>

using namespace serial_internal;

extern "C" auto serialDrain(int64_t handle_ptr, ErrorCallbackT error_callback) -> int
{
    auto *handle = reinterpret_cast<SerialPortHandle *>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError),
            "serialDrain: Invalid handle",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    std::scoped_lock const guard(handle->mtx);

    if (tcdrain(handle->fd) != 0)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kGetStateError), "serialDrain: tcdrain failed", error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kGetStateError);
    }
    return 0;
}
