#include <cpp_core/interface/serial_set_data_bits.h>
#include <cpp_core/status_codes.h>

#include "detail/posix_helpers.hpp"
#include "detail/posix_termios2.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialSetDataBits(int64_t handle, int data_bits, ErrorCallbackT error_callback) -> int
    {
        int fd = -1;
        const auto rc = cpp_bindings_linux::detail::validatePosixFd<int>(handle, error_callback, &fd);
        if (rc < 0)
        {
            return rc;
        }

        tcflag_t bits_flag = 0;
        switch (data_bits)
        {
        case 5:
            bits_flag = CS5;
            break;
        case 6:
            bits_flag = CS6;
            break;
        case 7:
            bits_flag = CS7;
            break;
        case 8:
            bits_flag = CS8;
            break;
        default:
            return cpp_bindings_linux::detail::failMsg<int>(error_callback, cpp_core::StatusCodes::kSetDataBitsError,
                                                            "Invalid data bits: must be 5-8");
        }

        struct termios2 tty = {};
        if (ioctl(fd, TCGETS2, &tty) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kGetStateError);
        }

        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= bits_flag;

        if (ioctl(fd, TCSETS2, &tty) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback,
                                                              cpp_core::StatusCodes::kSetDataBitsError);
        }

        return static_cast<int>(cpp_core::StatusCodes::kSuccess);
    }

} // extern "C"
