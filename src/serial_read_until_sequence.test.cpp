#include <cpp_core/interface/serial_close.h>
#include <cpp_core/interface/serial_read.h>
#include <cpp_core/interface/serial_read_until_sequence.h>
#include <cpp_core/status_code.h>

#include "detail/handle_types.hpp"

#include <array>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <unistd.h>

#include <gtest/gtest.h>

namespace
{
using cpp_bindings_linux::detail::UniqueFd;
constexpr auto kTimeout = cpp_core::SerialTimeoutConfig::make<10, 1>();
constexpr int kBufferError = static_cast<int>(cpp_core::StatusCode::Io::kBufferError);
} // namespace

TEST(SerialReadUntilSequenceTest, RejectsNullNegativeAndOverflowingTimeouts)
{
    constexpr int kTimeoutError = static_cast<int>(cpp_core::StatusCode::Configuration::kSetTimeoutError);
    cpp_bindings_linux::detail::UniqueFd fd(open("/dev/null", O_RDWR | O_NONBLOCK));
    ASSERT_TRUE(fd.valid());
    std::array<std::uint8_t, 4> buffer{};
    const auto check = [&](const cpp_core::SerialTimeoutConfig *timeout) {
        EXPECT_EQ(serialReadUntilSequence(fd.get(), buffer.data(), 4, timeout, buffer.data(), 1), kTimeoutError);
    };
    check(nullptr);
    for (const auto timeout : {cpp_core::SerialTimeoutConfig{-1, 1}, {1, -1}, {std::numeric_limits<int>::max(), 2}})
    {
        check(&timeout);
    }
}

TEST(SerialReadUntilSequenceTest, ReadsBinarySequenceWithEmbeddedZeroAndLeavesFollowingBytes)
{
    std::array<int, 2> fds{};
    ASSERT_EQ(pipe2(fds.data(), O_NONBLOCK | O_CLOEXEC), 0);
    UniqueFd reader(fds[0]);
    UniqueFd writer(fds[1]);
    constexpr std::array<std::uint8_t, 8> payload{'a', 'a', 0, 'a', 0, 'b', 'x', 'y'};
    constexpr std::array<std::uint8_t, 4> sequence{'a', 0, 'b', 'z'};
    ASSERT_EQ(write(writer.get(), payload.data(), payload.size()), static_cast<ssize_t>(payload.size()));
    std::array<std::uint8_t, 16> buffer{};
    // The explicit size excludes 'z'; the sequence has no C-string terminator.
    EXPECT_EQ(serialReadUntilSequence(reader.get(), buffer.data(), 16, &kTimeout, sequence.data(), 3), 6);
    EXPECT_EQ(std::memcmp(buffer.data(), payload.data(), 6), 0);
    EXPECT_EQ(serialRead(reader.get(), buffer.data(), 2, &kTimeout), 2);
    EXPECT_EQ(buffer[0], 'x');
    EXPECT_EQ(buffer[1], 'y');
    EXPECT_EQ(serialClose(reader.release()), 0);
}

TEST(SerialReadUntilSequenceTest, ValidatesSequenceSizeAndHonorsBufferCapacity)
{
    UniqueFd fd(open("/dev/null", O_RDONLY));
    ASSERT_TRUE(fd.valid());
    std::array<std::uint8_t, 4> buffer{};
    EXPECT_EQ(serialReadUntilSequence(fd.get(), buffer.data(), 4, &kTimeout, nullptr, 1), kBufferError);
    EXPECT_EQ(serialReadUntilSequence(fd.get(), buffer.data(), 4, &kTimeout, buffer.data(), 0), kBufferError);
    EXPECT_EQ(serialReadUntilSequence(fd.get(), buffer.data(), 4, &kTimeout, buffer.data(), -1), kBufferError);

    std::array<int, 2> fds{};
    ASSERT_EQ(pipe2(fds.data(), O_NONBLOCK | O_CLOEXEC), 0);
    UniqueFd reader(fds[0]);
    UniqueFd writer(fds[1]);
    constexpr std::array<std::uint8_t, 5> payload{'a', 'b', 'c', 'd', 'e'};
    ASSERT_EQ(write(writer.get(), payload.data(), payload.size()), static_cast<ssize_t>(payload.size()));
    EXPECT_EQ(serialReadUntilSequence(reader.get(), buffer.data(), 4, &kTimeout, payload.data(), 5), 4);
    EXPECT_EQ(std::memcmp(buffer.data(), payload.data(), 4), 0);
    EXPECT_EQ(serialClose(reader.release()), 0);
}
