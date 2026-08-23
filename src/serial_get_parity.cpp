#include <cpp_core/interface/serial_get_parity.h>

#include "detail/acquire_handle_context.hpp"
#include "detail/fail_errno.hpp"
#include "detail/status_value.hpp"
#include "detail/termios2.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialGetParity(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext handle_context;
        const auto status =
            cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &handle_context);
        if (status < 0)
        {
            return status;
        }

        termios2 serial_settings{};
        if (ioctl(handle_context.file_descriptor, TCGETS2, &serial_settings) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kGetStateError));
        }

        if ((serial_settings.c_cflag & PARENB) == 0)
        {
            return 0;
        }
        return (serial_settings.c_cflag & PARODD) != 0 ? 2 : 1;
    }

} // extern "C"
