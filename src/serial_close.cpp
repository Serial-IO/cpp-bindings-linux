#include "serial_internal.hpp"

#include <cpp_core/interface/serial_close.h>

using namespace serial_internal;

extern "C" int serialClose(
    int64_t        handle_ptr,
    ErrorCallbackT error_callback
)
{
    auto *handle = reinterpret_cast<SerialPortHandle *>(handle_ptr);
    if (handle == nullptr)
    {
        // Nothing to do – spec says it is a no-op, but we still report success.
        return 0;
    }

    std::lock_guard<std::recursive_mutex> guard(handle->mtx);

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
