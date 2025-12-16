#include <cpp_core/interface/serial_open.h>
#include <cpp_core/status_codes.h>

#include "detail/posix_helpers.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#ifndef TCGETS2
#define TCGETS2 0x802C542A
#define TCSETS2 0x402C542B
#endif

// Some libcs (or kernel headers) may not define BOTHER even if TCGETS2 exists.
// Define it here if missing so the build works in minimal environments
// (e.g., Deno's Debian-based CI containers).
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
        if (port == nullptr)
        {
            return cpp_bindings_linux::detail::failMsg<intptr_t>(error_callback, cpp_core::StatusCodes::kNotFoundError,
                                                                 "Port parameter is nullptr");
        }

        if (baudrate < 300)
        {
            return cpp_bindings_linux::detail::failMsg<intptr_t>(error_callback, cpp_core::StatusCodes::kSetStateError,
                                                                 "Invalid baudrate: must be >= 300");
        }

        if (data_bits < 5 || data_bits > 8)
        {
            return cpp_bindings_linux::detail::failMsg<intptr_t>(error_callback, cpp_core::StatusCodes::kSetStateError,
                                                                 "Invalid data bits: must be 5-8");
        }

        const char *port_path = static_cast<const char *>(port);

        cpp_bindings_linux::detail::UniqueFd handle(open(port_path, O_RDWR | O_NOCTTY | O_NONBLOCK));
        if (!handle.valid())
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
            return cpp_bindings_linux::detail::failMsg<intptr_t>(error_callback, cpp_core::StatusCodes::kSetStateError,
                                                                 "Invalid data bits");
        }

        tty.c_cflag &= ~(PARENB | PARODD);
        switch (parity)
        {
        case 0:
            break;
        case 1:
            tty.c_cflag |= PARENB;
            break;
        case 2:
            tty.c_cflag |= (PARENB | PARODD);
            break;
        default:
            return cpp_bindings_linux::detail::failMsg<intptr_t>(error_callback, cpp_core::StatusCodes::kSetStateError,
                                                                 "Invalid parity");
        }

        if (stop_bits == 2)
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

        int flags = fcntl(handle.get(), F_GETFL);
        if (flags < 0)
        {
            return cpp_bindings_linux::detail::failErrno<intptr_t>(error_callback,
                                                                   cpp_core::StatusCodes::kSetStateError);
        }
        flags &= ~O_NONBLOCK;
        const int set_flags_result = fcntl(handle.get(), F_SETFL, flags);
        if (set_flags_result != 0)
        {
            return cpp_bindings_linux::detail::failErrno<intptr_t>(error_callback,
                                                                   cpp_core::StatusCodes::kSetStateError);
        }

        tcflush(handle.get(), TCIOFLUSH);

        // Note: Some devices (e.g., Arduino) reset when the serial port is opened.
        // It is recommended to wait 1-2 seconds after opening before sending data
        // to allow the device to initialize.

        return static_cast<intptr_t>(handle.release());
    }

} // extern "C"
