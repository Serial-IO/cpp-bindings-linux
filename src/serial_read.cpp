#include <cpp_core/interface/serial_read.h>
#include <cpp_core/status_codes.h>

#include "detail/abort_registry.hpp"
#include "detail/posix_helpers.hpp"

#include <cerrno>
#include <limits>
#include <optional>
#include <unistd.h>

namespace
{
constexpr int kGraceRetryTimeoutMs = 10;

auto tryReadOnceNonBlocking(int fd, unsigned char *dst, int size) -> ssize_t
{
    const ssize_t bytes = ::read(fd, dst, size);
    if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
        return 0;
    }
    return bytes;
}

auto handleWaitResultForRead(int wait_result, int abort_fd, ErrorCallbackT error_callback) -> std::optional<int>
{
    if (wait_result < 0)
    {
        return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kReadError);
    }
    if (wait_result == 0)
    {
        return 0;
    }
    if (wait_result == 2)
    {
        cpp_bindings_linux::detail::drainNonBlockingFd(abort_fd);
        return static_cast<int>(cpp_core::StatusCodes::kAbortReadError);
    }
    return std::nullopt;
}

auto readUntilTimeoutOrFull(int fd, int abort_fd, unsigned char *buf, int buffer_size, int already_read,
                            int per_iteration_timeout_ms, ErrorCallbackT error_callback) -> int
{
    int total_read = already_read;
    while (total_read < buffer_size)
    {
        const int wait_result =
            cpp_bindings_linux::detail::waitFdReadyOrAbort(fd, abort_fd, per_iteration_timeout_ms, true);
        if (const auto immediate = handleWaitResultForRead(wait_result, abort_fd, error_callback);
            immediate.has_value())
        {
            // timeout -> return what we have; abort/error handled by helper
            if (*immediate == 0)
            {
                return total_read;
            }
            return *immediate;
        }

        const ssize_t more_bytes = tryReadOnceNonBlocking(fd, buf + total_read, buffer_size - total_read);
        if (more_bytes < 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kReadError);
        }
        if (more_bytes == 0)
        {
            // Driver reported readiness but returned nothing; treat like "no more data".
            return total_read;
        }
        total_read += static_cast<int>(more_bytes);
    }
    return total_read;
}
} // namespace

extern "C"
{
    MODULE_API auto serialRead(int64_t handle, void *buffer, int buffer_size, int timeout_ms, int multiplier,
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
        auto *buf = static_cast<unsigned char *>(buffer);

        const auto abort_pipes = cpp_bindings_linux::detail::getAbortPipesForFd(fd);
        const int abort_fd = abort_pipes ? abort_pipes->read_abort_r : -1;

        const int ready = cpp_bindings_linux::detail::waitFdReadyOrAbort(fd, abort_fd, timeout_ms, true);
        if (const auto immediate = handleWaitResultForRead(ready, abort_fd, error_callback); immediate.has_value())
        {
            return *immediate;
        }

        ssize_t bytes_read = tryReadOnceNonBlocking(fd, buf, buffer_size);
        if (bytes_read < 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kReadError);
        }

        // Some drivers can report readiness but still return 0; give it a tiny grace period and retry once.
        if (bytes_read == 0)
        {
            const int retry_ready =
                cpp_bindings_linux::detail::waitFdReadyOrAbort(fd, abort_fd, kGraceRetryTimeoutMs, true);
            if (const auto immediate = handleWaitResultForRead(retry_ready, abort_fd, error_callback);
                immediate.has_value())
            {
                return *immediate;
            }

            bytes_read = tryReadOnceNonBlocking(fd, buf, buffer_size);
            if (bytes_read <= 0)
            {
                return 0;
            }
        }

        const int already_read = static_cast<int>(bytes_read);

        if (multiplier == 0)
        {
            return already_read;
        }

        const int per_byte_timeout_ms = timeout_ms * multiplier;
        if (per_byte_timeout_ms <= 0)
        {
            return readUntilTimeoutOrFull(fd, abort_fd, buf, buffer_size, already_read, 0, error_callback);
        }

        return readUntilTimeoutOrFull(fd, abort_fd, buf, buffer_size, already_read, per_byte_timeout_ms,
                                      error_callback);
    }

} // extern "C"
