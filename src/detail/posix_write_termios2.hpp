#pragma once

#include "posix_fail_errno.hpp"
#include "posix_termios2.hpp"

#include <sys/ioctl.h>

namespace cpp_bindings_linux::detail
{
template <typename Ret>
inline auto writeTermios2(int file_descriptor, termios2 *serial_settings, ErrorCallbackT error_callback,
                          StatusCodeValue set_error_code) -> Ret
{
    if (ioctl(file_descriptor, TCSETS2, serial_settings) != 0)
    {
        return failErrno<Ret>(error_callback, set_error_code);
    }

    return static_cast<Ret>(StatusCode::kSuccess);
}
} // namespace cpp_bindings_linux::detail
