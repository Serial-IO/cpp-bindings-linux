#include <cpp_core/interface/serial_read.h>
#include <cpp_core/status_codes.h>

#include <cerrno>
#include <poll.h>
#include <system_error>
#include <unistd.h>

// NOLINTNEXTLINE(misc-use-anonymous-namespace)
static auto waitFdReady(int fd, int timeout_ms, bool for_read) -> int
{
    struct pollfd pfd = {};
    pfd.fd = fd;
    pfd.events = for_read ? POLLIN : POLLOUT;
    pfd.revents = 0;

    const int result = poll(&pfd, 1, timeout_ms);

    if (result < 0)
    {
        return -1;
    }
    if (result == 0)
    {
        return 0;
    }
    if (for_read && ((pfd.revents & POLLIN) != 0))
    {
        return 1;
    }
    if (!for_read && ((pfd.revents & POLLOUT) != 0))
    {
        return 1;
    }
    return 0;
}

extern "C"
{
    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    MODULE_API auto serialRead(int64_t handle, void *buffer, int buffer_size, int timeout_ms, int /*multiplier*/,
                               ErrorCallbackT error_callback) -> int
    {
        if (buffer == nullptr || buffer_size <= 0)
        {
            if (error_callback != nullptr)
            {
                error_callback(static_cast<int>(cpp_core::StatusCodes::kBufferError), "Invalid buffer or buffer_size");
            }
            return static_cast<int>(cpp_core::StatusCodes::kBufferError);
        }

        if (handle <= 0)
        {
            if (error_callback != nullptr)
            {
                error_callback(static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError), "Invalid handle");
            }
            return static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError);
        }

        const int fd = static_cast<int>(handle);
        auto *buf = static_cast<unsigned char *>(buffer);

        int elapsed_ms = 0;
        const int poll_interval = 10;
        bool data_ready = false;

        while (elapsed_ms < timeout_ms)
        {
            struct pollfd pfd = {};
            pfd.fd = fd;
            pfd.events = POLLIN;
            pfd.revents = 0;

            const int remaining_ms = timeout_ms - elapsed_ms;
            const int poll_timeout = (remaining_ms < poll_interval) ? remaining_ms : poll_interval;

            const int poll_result = poll(&pfd, 1, poll_timeout);
            if (poll_result > 0 && ((pfd.revents & POLLIN) != 0))
            {
                data_ready = true;
                break;
            }
            if (poll_result < 0)
            {
                return 0;
            }

            elapsed_ms += poll_interval;
        }

        if (!data_ready)
        {
            return 0;
        }

        ssize_t bytes_read = ::read(fd, buf, buffer_size);

        if (bytes_read < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return 0;
            }
            if (error_callback != nullptr)
            {
                const std::string error_msg = std::error_code(errno, std::generic_category()).message();
                error_callback(static_cast<int>(cpp_core::StatusCodes::kReadError), error_msg.c_str());
            }
            return static_cast<int>(cpp_core::StatusCodes::kReadError);
        }

        if (bytes_read == 0)
        {
            struct pollfd pfd = {};
            pfd.fd = fd;
            pfd.events = POLLIN;
            pfd.revents = 0;
            if (poll(&pfd, 1, 10) > 0 && ((pfd.revents & POLLIN) != 0))
            {
                bytes_read = ::read(fd, buf, buffer_size);
                if (bytes_read <= 0)
                {
                    return 0;
                }
            }
            else
            {
                return 0;
            }
        }

        int total_read = static_cast<int>(bytes_read);
        while (total_read < buffer_size)
        {
            struct pollfd pfd = {};
            pfd.fd = fd;
            pfd.events = POLLIN;
            pfd.revents = 0;

            if (poll(&pfd, 1, 0) <= 0 || ((pfd.revents & POLLIN) == 0))
            {
                break;
            }

            const ssize_t more_bytes = ::read(fd, buf + total_read, buffer_size - total_read);
            if (more_bytes <= 0)
            {
                break;
            }
            total_read += static_cast<int>(more_bytes);
        }

        return total_read;
    }

} // extern "C"
