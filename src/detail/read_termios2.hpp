#pragma once

#include "fail_errno.hpp"
#include "status_value.hpp"
#include "termios2.hpp"

#include <sys/ioctl.h>

namespace cpp_bindings_linux::detail
{
template <typename ReturnType>
inline auto readTermios2(int file_descriptor, termios2 *serial_settings, ErrorCallbackT error_callback) -> ReturnType
{
    if (ioctl(file_descriptor, TCGETS2, serial_settings) != 0)
    {
        return failErrno<ReturnType>(error_callback, statusValue(StatusCode::Control::kGetStateError));
    }

    return static_cast<ReturnType>(StatusCode::kSuccess);
}
} // namespace cpp_bindings_linux::detail
