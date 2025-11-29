#include "cpp_core/error_callback.h"
#include "cpp_core/status_codes.h"
#include "serial_internal.hpp"

#include <cpp_core/interface/serial_open.h>
#include <cstdint>
#include <fcntl.h>
#include <string_view>
#include <termios.h>
#include <unistd.h>
#include <utility>

using namespace serial_internal;

extern "C" auto serialOpen(void *port, int baudrate, int data_bits, int parity, int stop_bits,
                           ErrorCallbackT error_callback) -> intptr_t
{
    if (port == nullptr)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError),
            "serialOpen: Invalid port handle",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    std::string_view const port_name{static_cast<const char *>(port)};
    int const fd = open(port_name.data(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError),
            "serialOpen: Failed to open serial port",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    auto *handle = new SerialPortHandle{};
    handle->fd   = fd;

    termios tty{};
    if (tcgetattr(fd, &tty) != 0)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kGetStateError),
            "serialOpen: Failed to get port attributes",
            error_callback
        );
        close(fd);
        delete handle;
        return std::to_underlying(cpp_core::StatusCodes::kGetStateError);
    }

    handle->original = tty; // Save original settings

    // Local connection, enable receiver
    tty.c_cflag |= (CLOCAL | CREAD);

    // Baud rate
    const speed_t speed = to_speed_t(baudrate);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    // Data bits
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
    default:
        tty.c_cflag |= CS8;
        break;
    }

    // Parity
    if (parity == 0)
    {
        tty.c_cflag &= ~PARENB;
    }
    else
    {
        tty.c_cflag |= PARENB;
        if (parity == 1)
        {
            tty.c_cflag &= ~PARODD; // even
        }
        else
        {
            tty.c_cflag |= PARODD; // odd
        }
    }

    // Stop bits
    if (stop_bits == 2)
    {
        tty.c_cflag |= CSTOPB;
    }
    else
    {
        tty.c_cflag &= ~CSTOPB;
    }

    // Raw mode (no processing)
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;

    tty.c_cc[VMIN]  = 0; // Non-blocking by default
    tty.c_cc[VTIME] = 10; // 1 s read timeout

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kSetStateError),
            "serialOpen: Failed to configure port",
            error_callback
        );
        close(fd);
        delete handle;
        return std::to_underlying(cpp_core::StatusCodes::kSetStateError);
    }

    return reinterpret_cast<intptr_t>(handle);
}
