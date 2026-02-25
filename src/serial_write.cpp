#include <cpp_core/interface/serial_write.h>
#include <cpp_core/validation.hpp>

#include "detail/posix_helpers.hpp"

#include <cerrno>
#include <termios.h>
#include <unistd.h>

extern "C"
{

    MODULE_API auto serialWrite(int64_t handle, const void *buffer, int buffer_size, int timeout_ms, int /*multiplier*/,
                                ErrorCallbackT error_callback) -> int
    {
        const auto buf_ok = cpp_core::validateBuffer<int>(buffer, buffer_size, error_callback);
        if (buf_ok < 0)
        {
            return buf_ok;
        }

        const auto handle_ok = cpp_core::validateHandle<int>(handle, error_callback);
        if (handle_ok < 0)
        {
            return handle_ok;
        }

        timeout_ms = cpp_core::clampTimeout(timeout_ms);

        const int fd = static_cast<int>(handle);

        ssize_t bytes_written = ::write(fd, buffer, buffer_size);
        if (bytes_written < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                const int ready = cpp_bindings_linux::detail::waitFdReady(fd, timeout_ms, false);
                if (ready < 0)
                {
                    return cpp_bindings_linux::detail::failErrno<int>(error_callback,
                                                                      cpp_core::StatusCodes::kWriteError);
                }
                if (ready > 0)
                {
                    bytes_written = ::write(fd, buffer, buffer_size);
                }
                else
                {
                    return 0;
                }
            }

            if (bytes_written < 0)
            {
                return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kWriteError);
            }
        }

        tcdrain(fd);

        return static_cast<int>(bytes_written);
    }

} // extern "C"
