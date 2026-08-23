#pragma once

#include "termios2.hpp"

#include <termios.h>

namespace cpp_bindings_linux::detail
{
inline auto applyBaudrate(termios2 *serial_settings, int baudrate) -> void
{
    serial_settings->c_cflag &= ~CBAUD;
    serial_settings->c_cflag |= BOTHER;
    serial_settings->c_ispeed = static_cast<speed_t>(baudrate);
    serial_settings->c_ospeed = static_cast<speed_t>(baudrate);
}
} // namespace cpp_bindings_linux::detail
