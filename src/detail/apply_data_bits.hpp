#pragma once

#include "termios2.hpp"

#include <termios.h>

namespace cpp_bindings_linux::detail
{
inline auto applyDataBits(termios2 *serial_settings, int data_bits) -> void
{
    serial_settings->c_cflag &= ~CSIZE;
    switch (data_bits)
    {
    case 5:
        serial_settings->c_cflag |= CS5;
        break;
    case 6:
        serial_settings->c_cflag |= CS6;
        break;
    case 7:
        serial_settings->c_cflag |= CS7;
        break;
    case 8:
    default:
        serial_settings->c_cflag |= CS8;
        break;
    }
}
} // namespace cpp_bindings_linux::detail
