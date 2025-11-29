#include "cpp_core/error_callback.h"
#include "cpp_core/status_codes.h"
#include "serial_internal.hpp"

#include <cpp_core/interface/serial_clear_buffer_out.h>
#include <cstdint>
#include <mutex>
#include <termios.h>
#include <utility>

using serial_internal::invokeError;
using serial_internal::SerialPortHandle;

extern "C" auto serialClearBufferOut(
    int64_t        handle_ptr,
    ErrorCallbackT error_callback
) -> int
{
    auto *handle = reinterpret_cast<SerialPortHandle *>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError),
            "serialClearBufferOut: Invalid handle",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    std::scoped_lock const guard(handle->mtx);

    if (tcflush(handle->fd, TCOFLUSH) != 0)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kClearBufferOutError),
            "serialClearBufferOut: Failed to flush output buffer",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kClearBufferOutError);
    }
    return 0;
}
