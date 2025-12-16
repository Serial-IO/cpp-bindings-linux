#pragma once

#include <cpp_core/status_codes.h>

#include <cerrno>
#include <poll.h>
#include <string>
#include <system_error>
#include <unistd.h>

namespace cpp_bindings_linux::detail
{
class UniqueFd
{
  public:
    UniqueFd() = default;
    explicit UniqueFd(int in_fd) : fd_(in_fd)
    {
    }

    UniqueFd(const UniqueFd &) = delete;
    auto operator=(const UniqueFd &) -> UniqueFd & = delete;

    UniqueFd(UniqueFd &&other) noexcept : fd_(other.fd_)
    {
        other.fd_ = -1;
    }
    auto operator=(UniqueFd &&other) noexcept -> UniqueFd &
    {
        if (this != &other)
        {
            reset(other.release());
        }
        return *this;
    }

    ~UniqueFd()
    {
        reset(-1);
    }

    [[nodiscard]] auto get() const -> int
    {
        return fd_;
    }
    [[nodiscard]] auto valid() const -> bool
    {
        return fd_ >= 0;
    }

    auto reset(int new_fd) -> void
    {
        if (fd_ >= 0)
        {
            close(fd_);
        }
        fd_ = new_fd;
    }

    [[nodiscard]] auto release() -> int
    {
        const int out = fd_;
        fd_ = -1;
        return out;
    }

  private:
    int fd_ = -1;
};

template <typename Callback>
inline auto invokeErrorCallback(Callback error_callback, cpp_core::StatusCodes code, const char *message) -> void
{
    if (error_callback != nullptr)
    {
        error_callback(static_cast<int>(code), message);
    }
}

template <typename Ret, typename Callback>
inline auto failMsg(Callback error_callback, cpp_core::StatusCodes code, const char *message) -> Ret
{
    invokeErrorCallback(error_callback, code, message);
    return static_cast<Ret>(code);
}

template <typename Ret, typename Callback>
inline auto failErrno(Callback error_callback, cpp_core::StatusCodes code) -> Ret
{
    if (error_callback != nullptr)
    {
        const std::string error_msg = std::error_code(errno, std::generic_category()).message();
        error_callback(static_cast<int>(code), error_msg.c_str());
    }
    return static_cast<Ret>(code);
}

// Poll helper used by read/write to implement timeouts.
// Returns: -1 on poll error, 0 on timeout/not-ready, 1 on ready.
inline auto waitFdReady(int file_descriptor, int timeout_ms, bool for_read) -> int
{
    struct pollfd poll_fd = {};
    poll_fd.fd = file_descriptor;
    poll_fd.events = for_read ? POLLIN : POLLOUT;
    poll_fd.revents = 0;

    const int poll_result = poll(&poll_fd, 1, timeout_ms);
    if (poll_result < 0)
    {
        return -1;
    }
    if (poll_result == 0)
    {
        return 0;
    }
    if (for_read && ((poll_fd.revents & POLLIN) != 0))
    {
        return 1;
    }
    if (!for_read && ((poll_fd.revents & POLLOUT) != 0))
    {
        return 1;
    }
    return 0;
}
} // namespace cpp_bindings_linux::detail
