#pragma once

#include "posix_common_types.hpp"
#include "posix_termios2.hpp"

#include <termios.h>

namespace cpp_bindings_linux::detail
{
inline auto applyStopBits(termios2 *serial_settings, StopBits stop_bits) -> void
{
    if (stop_bits == StopBits::kTwo)
    {
        serial_settings->c_cflag |= CSTOPB;
    }
    else
    {
        serial_settings->c_cflag &= ~CSTOPB;
    }
}
} // namespace cpp_bindings_linux::detail
