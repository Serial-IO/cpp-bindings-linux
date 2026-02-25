#pragma once

#include <cpp_core/error_handling.hpp>
#include <cpp_core/unique_resource.hpp>

#include <cerrno>
#include <poll.h>
#include <string>
#include <system_error>
#include <unistd.h>

namespace cpp_bindings_linux::detail
{

// POSIX file descriptor traits for UniqueResource
struct PosixFdTraits
{
    using handle_type = int;

    static constexpr auto invalid() noexcept -> handle_type
    {
        return -1;
    }

    static auto close(handle_type fd) noexcept -> void
    {
        ::close(fd);
    }
};

using UniqueFd = cpp_core::UniqueResource<PosixFdTraits>;

// POSIX-specific error helper
template <cpp_core::StatusConvertible Ret, cpp_core::ErrorCallback Callback>
inline auto failErrno(Callback &&error_callback, cpp_core::StatusCodes code) -> Ret
{
    const std::string error_msg = std::error_code(errno, std::generic_category()).message();
    cpp_core::invokeError(std::forward<Callback>(error_callback), code, error_msg);
    return static_cast<Ret>(code);
}

// Poll helper
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
