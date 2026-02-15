#include <cpp_core/interface/serial_write.h>
#include <cpp_core/status_codes.h>

#include "detail/abort_registry.hpp"
#include "detail/posix_helpers.hpp"

#include <cerrno>
#include <limits>
#include <unistd.h>

extern "C"
{

    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    MODULE_API auto serialWrite(int64_t handle, const void *buffer, int buffer_size, int timeout_ms, int multiplier,
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

        const int fd = static_cast<int>(handle);
        const auto abort_pipes = cpp_bindings_linux::detail::getAbortPipesForFd(fd);
        const int abort_fd = abort_pipes ? abort_pipes->write_abort_r : -1;

        const auto *src = static_cast<const unsigned char *>(buffer);
        int total_written = 0;

        // Mirror cpp_core semantics: timeout_ms applies to the first byte,
        // then timeout_ms * multiplier applies to subsequent bytes.
        int current_timeout_ms = timeout_ms;

        while (total_written < buffer_size)
        {
            // Abort should also cancel writers that never hit EAGAIN/poll.
            if (cpp_bindings_linux::detail::consumeAbortIfSet(abort_fd))
            {
                return static_cast<int>(cpp_core::StatusCodes::kAbortWriteError);
            }

            const ssize_t num_written = ::write(fd, src + total_written, buffer_size - total_written);
            if (num_written > 0)
            {
                total_written += static_cast<int>(num_written);

                // If multiplier is 0, return immediately after the first successful write.
                if (multiplier == 0)
                {
                    return total_written;
                }
                // Subsequent bytes use scaled timeout.
                current_timeout_ms = timeout_ms * multiplier;
                continue;
            }

            if (num_written < 0 && errno == EINTR)
            {
                continue;
            }

            if (num_written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                const int ready =
                    cpp_bindings_linux::detail::waitFdReadyOrAbort(fd, abort_fd, current_timeout_ms, false);
                if (ready < 0)
                {
                    return cpp_bindings_linux::detail::failErrno<int>(error_callback,
                                                                      cpp_core::StatusCodes::kWriteError);
                }
                if (ready == 2)
                {
                    cpp_bindings_linux::detail::drainNonBlockingFd(abort_fd);
                    return static_cast<int>(cpp_core::StatusCodes::kAbortWriteError);
                }
                if (ready == 0)
                {
                    return total_written;
                }
                continue;
            }

            if (num_written == 0)
            {
                return total_written;
            }

            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kWriteError);
        }

        return total_written;
    }

} // extern "C"
