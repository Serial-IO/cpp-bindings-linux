#pragma once

#include <cpp_core/status_codes.h>

#include <cerrno>
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
} // namespace cpp_bindings_linux::detail
