#include <cpp_core/interface/serial_abort_read.h>
#include <cpp_core/interface/serial_abort_write.h>
#include <cpp_core/interface/serial_close.h>
#include <cpp_core/interface/serial_open.h>
#include <cpp_core/interface/serial_read.h>
#include <cpp_core/interface/serial_write.h>
#include <cpp_core/status_codes.h>
#include <gtest/gtest.h>

#include "../src/detail/abort_registry.hpp"

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

#include <fcntl.h>
#include <pty.h>
#include <unistd.h>

namespace
{
class UniqueFd
{
  public:
    explicit UniqueFd(int fd) : fd_(fd)
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
    [[nodiscard]] auto release() -> int
    {
        const int out = fd_;
        fd_ = -1;
        return out;
    }
    auto reset(int new_fd) -> void
    {
        if (fd_ >= 0)
        {
            (void)::close(fd_);
        }
        fd_ = new_fd;
    }

  private:
    int fd_ = -1;
};

auto fillNonBlockingFd(int fd) -> void
{
    const char buf[4096] = {};
    for (;;)
    {
        const ssize_t num_written = ::write(fd, buf, sizeof(buf));
        if (num_written > 0)
        {
            continue;
        }
        if (num_written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            return;
        }
        if (num_written < 0 && errno == EINTR)
        {
            continue;
        }
        return;
    }
}
} // namespace

TEST(AbortReadTest, AbortsBlockingReadFromOtherThread)
{
    int master_fd = -1;
    int slave_fd = -1;
    char slave_name[128] = {};
    ASSERT_EQ(::openpty(&master_fd, &slave_fd, slave_name, nullptr, nullptr), 0);
    UniqueFd master(master_fd);
    UniqueFd slave(slave_fd);

    // Use the path with serialOpen so we also test that abort pipes are registered on open().
    intptr_t handle = serialOpen(static_cast<void *>(slave_name), 115200, 8, 0, 0, nullptr);
    ASSERT_GT(handle, 0);

    std::atomic<int> read_result{999};
    std::thread reader([&] {
        unsigned char buffer[16] = {};
        // Long timeout to ensure we block in poll() until abort happens.
        read_result.store(serialRead(handle, buffer, sizeof(buffer), 10'000, 1, nullptr));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(serialAbortRead(handle, nullptr), 0);

    reader.join();
    EXPECT_EQ(read_result.load(), static_cast<int>(cpp_core::StatusCodes::kAbortReadError));

    EXPECT_EQ(serialClose(handle, nullptr), 0);
}

TEST(AbortWriteTest, AbortsBlockingWriteFromOtherThread)
{
    int pipe_fds[2] = {-1, -1};
    ASSERT_EQ(::pipe2(pipe_fds, O_NONBLOCK | O_CLOEXEC), 0);
    UniqueFd read_end(pipe_fds[0]);
    UniqueFd write_end(pipe_fds[1]);

    // Register abort pipes since we bypass serialOpen() here.
    ASSERT_TRUE(cpp_bindings_linux::detail::registerAbortPipesForFd(write_end.get()));

    // Fill the pipe buffer so future writes hit EAGAIN and serialWrite blocks in poll().
    fillNonBlockingFd(write_end.get());

    const char payload[4096] = {};
    std::atomic<int> write_result{999};
    std::thread writer([&] {
        write_result.store(
            serialWrite(write_end.get(), payload, static_cast<int>(sizeof(payload)), 10'000, 1, nullptr));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(serialAbortWrite(write_end.get(), nullptr), 0);

    writer.join();
    EXPECT_EQ(write_result.load(), static_cast<int>(cpp_core::StatusCodes::kAbortWriteError));

    cpp_bindings_linux::detail::unregisterAbortPipesForFd(write_end.get());
}
