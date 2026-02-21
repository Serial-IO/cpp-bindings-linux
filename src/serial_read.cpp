#include <cpp_core/interface/serial_read.h>
#include <cpp_core/status_codes.h>

#include "detail/posix_helpers.hpp"

#include <cerrno>
#include <limits>
#include <unistd.h>

extern "C"
{
    MODULE_API auto serialRead(int64_t handle, void *buffer, int buffer_size, int timeout_ms, int /*multiplier*/,
                               ErrorCallbackT error_callback) -> int
    {
        if (buffer == nullptr || buffer_size <= 0)
        {
            return cpp_bindings_linux::detail::failMsg<int>(error_callback, cpp_core::StatusCodes::kBufferError,
                                                            "Invalid buffer or buffer_size");
        }

        if (handle <= 0 || handle > std::numeric_limits<int>::max())
        {
            return cpp_bindings_linux::detail::failMsg<int>(error_callback, cpp_core::StatusCodes::kInvalidHandleError,
                                                            "Invalid handle");
        }

        if (timeout_ms < 0)
        {
            timeout_ms = 0;
        }

        const int fd = static_cast<int>(handle);
        auto *buf = static_cast<unsigned char *>(buffer);

        const int ready = cpp_bindings_linux::detail::waitFdReady(fd, timeout_ms, true);
        if (ready < 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kReadError);
        }
        if (ready == 0)
        {
            return 0;
        }

        const auto try_read_once = [&](unsigned char *dst, int size) -> ssize_t {
            const ssize_t bytes = ::read(fd, dst, size);
            if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                return 0;
            }
            return bytes;
        };

        ssize_t bytes_read = try_read_once(buf, buffer_size);
        if (bytes_read < 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kReadError);
        }

        // Some drivers can report readiness but still return 0; give it a tiny grace period and retry once.
        if (bytes_read == 0)
        {
            const int retry_ready = cpp_bindings_linux::detail::waitFdReady(fd, 10, true);
            if (retry_ready < 0)
            {
                return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kReadError);
            }
            if (retry_ready == 0)
            {
                return 0;
            }
            bytes_read = try_read_once(buf, buffer_size);
            if (bytes_read <= 0)
            {
                return 0;
            }
        }

        int total_read = static_cast<int>(bytes_read);
        while (total_read < buffer_size)
        {
            const int loop_ready = cpp_bindings_linux::detail::waitFdReady(fd, 0, true);
            if (loop_ready < 0)
            {
                return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kReadError);
            }
            if (loop_ready == 0)
            {
                break;
            }
            const ssize_t more_bytes = try_read_once(buf + total_read, buffer_size - total_read);
            if (more_bytes <= 0)
            {
                break;
            }
            total_read += static_cast<int>(more_bytes);
        }

        return total_read;
    }

} // extern "C"
