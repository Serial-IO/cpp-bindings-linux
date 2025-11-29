#include "cpp_core/error_callback.h"
#include "cpp_core/status_codes.h"
#include "serial_internal.hpp"

#include <cpp_core/interface/serial_clear_buffer_in.h>
#include <cstdint>
#include <mutex>
#include <termios.h>
#include <utility>

using namespace serial_internal;

extern "C" auto serialClearBufferIn(int64_t handle_ptr, ErrorCallbackT error_callback) -> int
{
    auto *handle = reinterpret_cast<SerialPortHandle *>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError),
            "serialClearBufferIn: Invalid handle",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    std::scoped_lock const guard(handle->mtx);

    if (tcflush(handle->fd, TCIFLUSH) != 0)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kClearBufferInError),
            "serialClearBufferIn: Failed to flush input buffer",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kClearBufferInError);
    }
    return 0;
}
