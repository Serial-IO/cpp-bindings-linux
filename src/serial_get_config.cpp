#include <cpp_core/interface/serial_get_config.h>
#include <cpp_core/status_codes.h>

#include "detail/posix_helpers.hpp"
#include "detail/posix_termios2.hpp"

#include <sys/ioctl.h>

namespace
{
auto dataBitsFromCflag(tcflag_t cflag) -> int
{
    switch (cflag & CSIZE)
    {
    case CS5:
        return 5;
    case CS6:
        return 6;
    case CS7:
        return 7;
    case CS8:
        return 8;
    default:
        return 8;
    }
}

auto parityFromFlags(tcflag_t cflag) -> int
{
    if ((cflag & PARENB) == 0)
    {
        return 0;
    }
    return (cflag & PARODD) ? 2 : 1;
}

auto flowControlFromFlags(tcflag_t cflag, tcflag_t iflag) -> int
{
    if ((cflag & CRTSCTS) != 0)
    {
        return 1;
    }
    if ((iflag & (IXON | IXOFF)) != 0)
    {
        return 2;
    }
    return 0;
}
} // namespace

extern "C"
{

    MODULE_API auto serialGetConfig(int64_t handle,
                                    void (*callback_fn)(int baudrate, int data_bits, int parity, int stop_bits,
                                                        int flow_control),
                                    ErrorCallbackT error_callback) -> int
    {
        int fd = -1;
        const auto rc = cpp_bindings_linux::detail::validatePosixFd<int>(handle, error_callback, &fd);
        if (rc < 0)
        {
            return rc;
        }

        if (callback_fn == nullptr)
        {
            return cpp_bindings_linux::detail::failMsg<int>(error_callback, cpp_core::StatusCodes::kGetConfigError,
                                                            "Callback must not be nullptr");
        }

        struct termios2 tty = {};
        if (ioctl(fd, TCGETS2, &tty) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kGetConfigError);
        }

        const int baudrate = static_cast<int>(tty.c_ospeed);
        const int data_bits = dataBitsFromCflag(tty.c_cflag);
        const int parity = parityFromFlags(tty.c_cflag);
        const int stop_bits = (tty.c_cflag & CSTOPB) ? 2 : 0;
        const int flow_control = flowControlFromFlags(tty.c_cflag, tty.c_iflag);

        callback_fn(baudrate, data_bits, parity, stop_bits, flow_control);

        return static_cast<int>(cpp_core::StatusCodes::kSuccess);
    }

} // extern "C"
