#include <cpp_core/interface/serial_update_baudrate.h>
#include <cpp_core/status_codes.h>

#include "detail/posix_helpers.hpp"
#include "detail/posix_termios2.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialUpdateBaudrate(int64_t handle, int baudrate, ErrorCallbackT error_callback) -> int
    {
        int fd = -1;
        const auto rc = cpp_bindings_linux::detail::validatePosixFd<int>(handle, error_callback, &fd);
        if (rc < 0)
        {
            return rc;
        }

        if (baudrate < 300)
        {
            return cpp_bindings_linux::detail::failMsg<int>(error_callback,
                                                            cpp_core::StatusCodes::kUpdateBaudrateError,
                                                            "Invalid baudrate: must be >= 300");
        }

        struct termios2 tty = {};
        if (ioctl(fd, TCGETS2, &tty) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kGetStateError);
        }

        tty.c_cflag &= ~CBAUD;
        tty.c_cflag |= BOTHER;
        tty.c_ispeed = baudrate;
        tty.c_ospeed = baudrate;

        if (ioctl(fd, TCSETS2, &tty) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback,
                                                              cpp_core::StatusCodes::kUpdateBaudrateError);
        }

        return static_cast<int>(cpp_core::StatusCodes::kSuccess);
    }

} // extern "C"
