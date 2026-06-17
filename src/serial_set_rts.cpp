#include <cpp_core/interface/serial_set_rts.h>

#include "detail/posix_helpers.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialSetRts(int64_t handle, int state, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext context;
        const auto rc = cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &context);
        if (rc < 0)
        {
            return rc;
        }

        int flag = TIOCM_RTS;
        if (ioctl(context.fd, state ? TIOCMBIS : TIOCMBIC, &flag) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kSetRtsError));
        }

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
