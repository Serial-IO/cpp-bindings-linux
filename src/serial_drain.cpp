#include <cpp_core/interface/serial_drain.h>

#include "detail/posix_helpers.hpp"

#include <termios.h>

extern "C"
{

    MODULE_API auto serialDrain(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext context;
        const auto rc = cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &context);
        if (rc < 0)
        {
            return rc;
        }

        if (tcdrain(context.fd) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Io::kWriteError));
        }

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
