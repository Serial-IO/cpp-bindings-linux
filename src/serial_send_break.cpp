#include <cpp_core/interface/serial_send_break.h>

#include "detail/posix_acquire_handle_context.hpp"
#include "detail/posix_fail_errno.hpp"
#include "detail/posix_fail_msg.hpp"
#include "detail/posix_status_value.hpp"

#include <sys/ioctl.h>
#include <unistd.h>

extern "C"
{

    MODULE_API auto serialSendBreak(int64_t handle, int duration_ms, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext handle_context;
        const auto status =
            cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &handle_context);
        if (status < 0)
        {
            return status;
        }

        if (duration_ms <= 0)
        {
            return cpp_bindings_linux::detail::failMsg<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kSendBreakError),
                "Break duration must be > 0");
        }

        if (ioctl(handle_context.file_descriptor, TIOCSBRK) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kSendBreakError));
        }

        usleep(static_cast<useconds_t>(duration_ms) * 1000U);

        if (ioctl(handle_context.file_descriptor, TIOCCBRK) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kSendBreakError));
        }

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
