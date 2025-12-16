#include <cpp_core/interface/serial_write.h>
#include <cpp_core/status_codes.h>

#include "detail/abort_registry.hpp"
#include "detail/posix_helpers.hpp"

#include <cerrno>
#include <fcntl.h>
#include <limits>
#include <unistd.h>

extern "C"
{

    MODULE_API auto serialWrite(int64_t handle, const void *buffer, int buffer_size, int timeout_ms, int /*multiplier*/,
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

        ssize_t bytes_written = ::write(fd, buffer, buffer_size);
        if (bytes_written < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                const int ready = cpp_bindings_linux::detail::waitFdReadyOrAbort(fd, abort_fd, timeout_ms, false);
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

        return static_cast<int>(bytes_written);
    }

} // extern "C"
