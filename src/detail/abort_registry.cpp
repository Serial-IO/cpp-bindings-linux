#include "abort_registry.hpp"

#include <array>
#include <fcntl.h>
#include <mutex>
#include <unistd.h>
#include <unordered_map>

namespace cpp_bindings_linux::detail
{
namespace
{
auto makePipeNonBlockingCloexec(std::array<int, 2> &out_pipe) -> bool
{
#ifdef __linux__
    // pipe2 is Linux-specific, which is fine for this bindings library.
    if (::pipe2(out_pipe.data(), O_NONBLOCK | O_CLOEXEC) == 0)
    {
        return true;
    }
#endif

    if (::pipe(out_pipe.data()) != 0)
    {
        return false;
    }

    for (int index = 0; index < 2; ++index)
    {
        const auto pipe_index = static_cast<size_t>(index);
        const int flags = ::fcntl(out_pipe[pipe_index], F_GETFL, 0);
        if (flags >= 0)
        {
            (void)::fcntl(out_pipe[pipe_index], F_SETFL, flags | O_NONBLOCK);
        }
        const int fd_flags = ::fcntl(out_pipe[pipe_index], F_GETFD, 0);
        if (fd_flags >= 0)
        {
            (void)::fcntl(out_pipe[pipe_index], F_SETFD, fd_flags | FD_CLOEXEC);
        }
    }
    return true;
}

auto closeIfValid(int fd) -> void
{
    if (fd >= 0)
    {
        (void)::close(fd);
    }
}

std::mutex g_abort_mutex;
std::unordered_map<int, AbortPipes> g_abort_by_fd;
} // namespace

auto registerAbortPipesForFd(int fd) -> bool
{
    if (fd < 0)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_abort_mutex);
    if (g_abort_by_fd.contains(fd))
    {
        return true;
    }

    std::array<int, 2> read_pipe = {-1, -1};
    std::array<int, 2> write_pipe = {-1, -1};
    if (!makePipeNonBlockingCloexec(read_pipe) || !makePipeNonBlockingCloexec(write_pipe))
    {
        closeIfValid(read_pipe[0]);
        closeIfValid(read_pipe[1]);
        closeIfValid(write_pipe[0]);
        closeIfValid(write_pipe[1]);
        return false;
    }

    AbortPipes pipes;
    pipes.read_abort_r = read_pipe[0];
    pipes.read_abort_w = read_pipe[1];
    pipes.write_abort_r = write_pipe[0];
    pipes.write_abort_w = write_pipe[1];

    g_abort_by_fd.emplace(fd, pipes);
    return true;
}

auto unregisterAbortPipesForFd(int fd) -> void
{
    if (fd < 0)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(g_abort_mutex);
    auto iter = g_abort_by_fd.find(fd);
    if (iter == g_abort_by_fd.end())
    {
        return;
    }

    closeIfValid(iter->second.read_abort_r);
    closeIfValid(iter->second.read_abort_w);
    closeIfValid(iter->second.write_abort_r);
    closeIfValid(iter->second.write_abort_w);

    g_abort_by_fd.erase(iter);
}

auto getAbortPipesForFd(int fd) -> std::optional<AbortPipes>
{
    if (fd < 0)
    {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(g_abort_mutex);
    auto iter = g_abort_by_fd.find(fd);
    if (iter == g_abort_by_fd.end())
    {
        return std::nullopt;
    }
    return iter->second;
}
} // namespace cpp_bindings_linux::detail
