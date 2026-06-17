#include <cpp_core/interface/serial_get_dcd.h>

#include "detail/posix_helpers.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialGetDcd(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext context;
        const auto rc = cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &context);
        if (rc < 0)
        {
            return rc;
        }

        int status = 0;
        if (ioctl(context.fd, TIOCMGET, &status) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback,
                cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kGetModemStatusError));
        }

        return (status & TIOCM_CAR) != 0 ? 1 : 0;
    }

} // extern "C"
