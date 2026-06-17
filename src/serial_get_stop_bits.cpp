#include <cpp_core/interface/serial_get_stop_bits.h>

#include "detail/posix_helpers.hpp"
#include "detail/posix_termios2.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialGetStopBits(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext context;
        const auto rc = cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &context);
        if (rc < 0)
        {
            return rc;
        }

        termios2 tty{};
        if (ioctl(context.fd, TCGETS2, &tty) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kGetStateError));
        }

        return (tty.c_cflag & CSTOPB) != 0 ? 2 : 0;
    }

} // extern "C"
