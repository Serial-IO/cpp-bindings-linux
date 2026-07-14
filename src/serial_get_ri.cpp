#include <cpp_core/interface/serial_get_ri.h>

#include "detail/posix_acquire_handle_context.hpp"
#include "detail/posix_fail_errno.hpp"
#include "detail/posix_status_value.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialGetRi(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext handle_context;
        const auto status =
            cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &handle_context);
        if (status < 0)
        {
            return status;
        }

        int modem_status = 0;
        if (ioctl(handle_context.file_descriptor, TIOCMGET, &modem_status) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback,
                cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kGetModemStatusError));
        }

        return (modem_status & TIOCM_RNG) != 0 ? 1 : 0;
    }

} // extern "C"
