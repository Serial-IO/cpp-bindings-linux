#include <cpp_core/interface/serial_write.h>
#include <cpp_core/status_codes.h>

#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <system_error>
#include <termios.h>
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

    MODULE_API auto serialWrite(int64_t handle, const void *buffer, int buffer_size, int timeout_ms, int /*multiplier*/,
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

        ssize_t bytes_written = ::write(fd, buffer, buffer_size);
        if (bytes_written < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                if (waitFdReady(fd, timeout_ms, false) > 0)
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
                if (error_callback != nullptr)
                {
                    const std::string error_msg = std::error_code(errno, std::generic_category()).message();
                    error_callback(static_cast<int>(cpp_core::StatusCodes::kWriteError), error_msg.c_str());
                }
                return static_cast<int>(cpp_core::StatusCodes::kWriteError);
            }
        }

        tcdrain(fd);

        return static_cast<int>(bytes_written);
    }

} // extern "C"
