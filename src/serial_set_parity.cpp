#include <cpp_core/interface/serial_set_parity.h>
#include <cpp_core/status_codes.h>

#include "detail/posix_helpers.hpp"
#include "detail/posix_termios2.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialSetParity(int64_t handle, int parity, ErrorCallbackT error_callback) -> int
    {
        int fd = -1;
        const auto rc = cpp_bindings_linux::detail::validatePosixFd<int>(handle, error_callback, &fd);
        if (rc < 0)
        {
            return rc;
        }

        if (parity < 0 || parity > 2)
        {
            return cpp_bindings_linux::detail::failMsg<int>(error_callback, cpp_core::StatusCodes::kSetParityError,
                                                            "Invalid parity: must be 0, 1, or 2");
        }

        struct termios2 tty = {};
        if (ioctl(fd, TCGETS2, &tty) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kGetStateError);
        }

        tty.c_cflag &= ~(PARENB | PARODD);
        switch (parity)
        {
        case 1:
            tty.c_cflag |= PARENB;
            break;
        case 2:
            tty.c_cflag |= (PARENB | PARODD);
            break;
        default:
            break;
        }

        if (ioctl(fd, TCSETS2, &tty) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kSetParityError);
        }

        return static_cast<int>(cpp_core::StatusCodes::kSuccess);
    }

} // extern "C"
