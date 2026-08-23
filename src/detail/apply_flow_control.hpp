#pragma once

#include "common_types.hpp"
#include "termios2.hpp"

#include <termios.h>

namespace cpp_bindings_linux::detail
{
inline auto applyFlowControl(termios2 *serial_settings, FlowControl flow_control) -> void
{
    serial_settings->c_cflag &= ~CRTSCTS;
    serial_settings->c_iflag &= ~(IXON | IXOFF | IXANY);

    if (flow_control == FlowControl::kRtsCts)
    {
        serial_settings->c_cflag |= CRTSCTS;
    }
    else if (flow_control == FlowControl::kXonXoff)
    {
        serial_settings->c_iflag |= (IXON | IXOFF);
    }
}
} // namespace cpp_bindings_linux::detail
