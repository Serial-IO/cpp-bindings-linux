#include "cpp_core/error_callback.h"
#include "cpp_core/status_codes.h"
#include "serial_internal.hpp"

#include <cpp_core/interface/serial_close.h>
#include <cstdint>
#include <mutex>
#include <termios.h>
#include <unistd.h>
#include <utility>

using namespace serial_internal;

extern "C" auto serialClose(int64_t handle_ptr, ErrorCallbackT error_callback) -> int
{
    auto *handle = reinterpret_cast<SerialPortHandle *>(handle_ptr);
    if (handle == nullptr)
    {
        // Nothing to do – spec says it is a no-op, but we still report success.
        return 0;
    }

    std::scoped_lock const guard(handle->mtx);

    // Restore original settings and close the descriptor.
    if (tcsetattr(handle->fd, TCSANOW, &handle->original) != 0)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kSetStateError),
            "serialClose: Failed to restore attributes",
            error_callback
        );
        // Continue with close anyway.
    }

    if (close(handle->fd) != 0)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kCloseHandleError),
            "serialClose: Failed to close port",
            error_callback
        );
        delete handle;
        return std::to_underlying(cpp_core::StatusCodes::kCloseHandleError);
    }

    delete handle;
    return 0;
}
