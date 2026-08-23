#pragma once

#include "common_types.hpp"
#include "termios2.hpp"

#include <termios.h>

namespace cpp_bindings_linux::detail
{
inline auto applyParity(termios2 *serial_settings, Parity parity) -> void
{
    serial_settings->c_cflag &= ~(PARENB | PARODD);
    if (parity == Parity::kEven)
    {
        serial_settings->c_cflag |= PARENB;
    }
    else if (parity == Parity::kOdd)
    {
        serial_settings->c_cflag |= (PARENB | PARODD);
    }
}
} // namespace cpp_bindings_linux::detail
