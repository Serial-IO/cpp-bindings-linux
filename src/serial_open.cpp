#include <cpp_core/interface/serial_open.h>
#include <cpp_core/strong_types.hpp>
#include <cpp_core/validation.hpp>

#include "detail/posix_helpers.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#ifndef TCGETS2
#define TCGETS2 0x802C542A
#define TCSETS2 0x402C542B
#endif

#ifndef BOTHER
#define BOTHER 0x010000
#endif

// NOLINTBEGIN
// C Structure is defined by the kernel, so we cannot change it.
struct termios2
{
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_line;
    cc_t c_cc[19];
    speed_t c_ispeed;
    speed_t c_ospeed;
};
// NOLINTEND

extern "C"
{
    MODULE_API auto serialOpen(void *port, int baudrate, int data_bits, int parity, int stop_bits,
                               ErrorCallbackT error_callback) -> intptr_t
    {
        const auto params_ok = cpp_core::validateOpenParams<intptr_t>(port, baudrate, data_bits, error_callback);
        if (params_ok < 0)
        {
            return params_ok;
        }

        const char *port_path = static_cast<const char *>(port);

        cpp_bindings_linux::detail::UniqueFd handle(open(port_path, O_RDWR | O_NOCTTY | O_NONBLOCK));
        if (!handle)
        {
            return cpp_bindings_linux::detail::failErrno<intptr_t>(error_callback,
                                                                   cpp_core::StatusCodes::kNotFoundError);
        }

        struct termios2 tty = {};
        if (ioctl(handle.get(), TCGETS2, &tty) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<intptr_t>(error_callback,
                                                                   cpp_core::StatusCodes::kGetStateError);
        }

        tty.c_cflag &= ~CBAUD;
        tty.c_cflag |= BOTHER;
        tty.c_ispeed = baudrate;
        tty.c_ospeed = baudrate;

        tty.c_cflag &= ~CSIZE;
        switch (data_bits)
        {
        case 5:
            tty.c_cflag |= CS5;
            break;
        case 6:
            tty.c_cflag |= CS6;
            break;
        case 7:
            tty.c_cflag |= CS7;
            break;
        case 8:
            tty.c_cflag |= CS8;
            break;
        default:
            return cpp_core::failMsg<intptr_t>(error_callback, cpp_core::StatusCodes::kSetStateError,
                                               "Invalid data bits");
        }

        const auto par = static_cast<cpp_core::Parity>(parity);
        tty.c_cflag &= ~(PARENB | PARODD);
        switch (par)
        {
        case cpp_core::Parity::kNone:
            break;
        case cpp_core::Parity::kEven:
            tty.c_cflag |= PARENB;
            break;
        case cpp_core::Parity::kOdd:
            tty.c_cflag |= (PARENB | PARODD);
            break;
        default:
            return cpp_core::failMsg<intptr_t>(error_callback, cpp_core::StatusCodes::kSetStateError, "Invalid parity");
        }

        // stop_bits: 0 or 1 = one stop bit (0 kept for backward compat), 2 = two stop bits
        if (stop_bits != static_cast<int>(cpp_core::StopBits::kOne) && stop_bits != 1 &&
            stop_bits != static_cast<int>(cpp_core::StopBits::kTwo))
        {
            return cpp_core::failMsg<intptr_t>(error_callback, cpp_core::StatusCodes::kSetStateError,
                                               "Invalid stop bits: must be 0, 1, or 2");
        }
        const auto sb = (stop_bits == static_cast<int>(cpp_core::StopBits::kTwo)) ? cpp_core::StopBits::kTwo
                                                                                   : cpp_core::StopBits::kOne;
        if (sb == cpp_core::StopBits::kTwo)
        {
            tty.c_cflag |= CSTOPB;
        }
        else
        {
            tty.c_cflag &= ~CSTOPB;
        }

        tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        tty.c_iflag &= ~(IXON | IXOFF | IXANY | INLCR | IGNCR | ICRNL);
        tty.c_oflag &= ~OPOST;

        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 0;

        if (ioctl(handle.get(), TCSETS2, &tty) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<intptr_t>(error_callback,
                                                                   cpp_core::StatusCodes::kSetStateError);
        }

        tcflush(handle.get(), TCIOFLUSH);

        return static_cast<intptr_t>(handle.release());
    }

} // extern "C"
