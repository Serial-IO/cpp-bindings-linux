#include <cpp_core/interface/serial_clear_buffer_in.h>

#include "detail/posix_helpers.hpp"

#include <termios.h>

extern "C"
{

    MODULE_API auto serialClearBufferIn(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext context;
        const auto rc = cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &context);
        if (rc < 0)
        {
            return rc;
        }

        if (tcflush(context.fd, TCIFLUSH) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Io::kClearBufferInError));
        }

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
