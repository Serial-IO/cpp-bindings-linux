#include <cpp_core/interface/serial_send_break.h>
#include <cpp_core/status_codes.h>

#include "detail/posix_helpers.hpp"

#include <sys/ioctl.h>
#include <unistd.h>

extern "C"
{

    MODULE_API auto serialSendBreak(int64_t handle, int duration_ms, ErrorCallbackT error_callback) -> int
    {
        int fd = -1;
        const auto rc = cpp_bindings_linux::detail::validatePosixFd<int>(handle, error_callback, &fd);
        if (rc < 0)
        {
            return rc;
        }

        if (duration_ms <= 0)
        {
            return cpp_bindings_linux::detail::failMsg<int>(error_callback, cpp_core::StatusCodes::kSendBreakError,
                                                            "Break duration must be > 0");
        }

        if (ioctl(fd, TIOCSBRK) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kSendBreakError);
        }

        usleep(static_cast<useconds_t>(duration_ms) * 1000);

        if (ioctl(fd, TIOCCBRK) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kSendBreakError);
        }

        return static_cast<int>(cpp_core::StatusCodes::kSuccess);
    }

} // extern "C"
