#include <cpp_core/interface/serial_send_break.h>

#include "detail/posix_helpers.hpp"

#include <sys/ioctl.h>
#include <unistd.h>

extern "C"
{

    MODULE_API auto serialSendBreak(int64_t handle, int duration_ms, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext context;
        const auto rc = cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &context);
        if (rc < 0)
        {
            return rc;
        }

        if (duration_ms <= 0)
        {
            return cpp_bindings_linux::detail::failMsg<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kSendBreakError),
                "Break duration must be > 0");
        }

        if (ioctl(context.fd, TIOCSBRK) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kSendBreakError));
        }

        usleep(static_cast<useconds_t>(duration_ms) * 1000U);

        if (ioctl(context.fd, TIOCCBRK) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kSendBreakError));
        }

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
