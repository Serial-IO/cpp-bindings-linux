#include <cpp_core/interface/serial_open.h>
#include <cpp_core/status_codes.h>

#include <cerrno>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <system_error>
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
    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    MODULE_API auto serialOpen(void *port, int baudrate, int data_bits, int parity, int stop_bits,
                               ErrorCallbackT error_callback) -> intptr_t
    {
        if (port == nullptr)
        {
            if (error_callback != nullptr)
            {
                error_callback(static_cast<int>(cpp_core::StatusCodes::kNotFoundError), "Port parameter is nullptr");
            }
            return static_cast<intptr_t>(cpp_core::StatusCodes::kNotFoundError);
        }

        if (baudrate < 300)
        {
            if (error_callback != nullptr)
            {
                error_callback(static_cast<int>(cpp_core::StatusCodes::kSetStateError),
                               "Invalid baudrate: must be >= 300");
            }
            return static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError);
        }

        if (data_bits < 5 || data_bits > 8)
        {
            if (error_callback != nullptr)
            {
                error_callback(static_cast<int>(cpp_core::StatusCodes::kSetStateError),
                               "Invalid data bits: must be 5-8");
            }
            return static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError);
        }

        const char *port_path = static_cast<const char *>(port);

        const int fd = open(port_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0)
        {
            if (error_callback != nullptr)
            {
                const std::string error_msg = std::error_code(errno, std::generic_category()).message();
                error_callback(static_cast<int>(cpp_core::StatusCodes::kNotFoundError), error_msg.c_str());
            }
            return static_cast<intptr_t>(cpp_core::StatusCodes::kNotFoundError);
        }

        struct termios2 tty = {};
        if (ioctl(fd, TCGETS2, &tty) != 0)
        {
            close(fd);
            if (error_callback != nullptr)
            {
                const std::string error_msg = std::error_code(errno, std::generic_category()).message();
                error_callback(static_cast<int>(cpp_core::StatusCodes::kGetStateError), error_msg.c_str());
            }
            return static_cast<intptr_t>(cpp_core::StatusCodes::kGetStateError);
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
            close(fd);
            if (error_callback != nullptr)
            {
                error_callback(static_cast<int>(cpp_core::StatusCodes::kSetStateError), "Invalid data bits");
            }
            return static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError);
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
            close(fd);
            if (error_callback != nullptr)
            {
                error_callback(static_cast<int>(cpp_core::StatusCodes::kSetStateError), "Invalid parity");
            }
            return static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError);
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

        if (ioctl(fd, TCSETS2, &tty) != 0)
        {
            close(fd);
            if (error_callback != nullptr)
            {
                const std::string error_msg = std::error_code(errno, std::generic_category()).message();
                error_callback(static_cast<int>(cpp_core::StatusCodes::kSetStateError), error_msg.c_str());
            }
            return static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError);
        }

        int flags = fcntl(fd, F_GETFL);
        if (flags < 0)
        {
            close(fd);
            if (error_callback != nullptr)
            {
                const std::string error_msg = std::error_code(errno, std::generic_category()).message();
                error_callback(static_cast<int>(cpp_core::StatusCodes::kSetStateError), error_msg.c_str());
            }
            return static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError);
        }
        flags &= ~O_NONBLOCK;
        const int set_flags_result = fcntl(fd, F_SETFL, flags);
        if (set_flags_result != 0)
        {
            close(fd);
            if (error_callback != nullptr)
            {
                const std::string error_msg = std::error_code(errno, std::generic_category()).message();
                error_callback(static_cast<int>(cpp_core::StatusCodes::kSetStateError), error_msg.c_str());
            }
            return static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError);
        }

        tcflush(fd, TCIOFLUSH);

        // Note: Some devices (e.g., Arduino) reset when the serial port is opened.
        // It is recommended to wait 1-2 seconds after opening before sending data
        // to allow the device to initialize.

        return static_cast<intptr_t>(fd);
    }

} // extern "C"
