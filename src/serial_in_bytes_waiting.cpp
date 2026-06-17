#include <cpp_core/interface/serial_in_bytes_waiting.h>

#include "detail/posix_helpers.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialInBytesWaiting(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext context;
        const auto rc = cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &context);
        if (rc < 0)
        {
            return rc;
        }

        int bytes_waiting = 0;
        if (ioctl(context.fd, FIONREAD, &bytes_waiting) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kGetStateError));
        }

        return bytes_waiting;
    }

} // extern "C"
