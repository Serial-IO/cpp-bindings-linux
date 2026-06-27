#include <cpp_core/interface/serial_set_dtr.h>

#include "detail/posix_helpers.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialSetDtr(int64_t handle, int state, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext handle_context;
        const auto status =
            cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &handle_context);
        if (status < 0)
        {
            return status;
        }

        int modem_flag = TIOCM_DTR;
        if (ioctl(handle_context.file_descriptor, state ? TIOCMBIS : TIOCMBIC, &modem_flag) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kSetDtrError));
        }

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
