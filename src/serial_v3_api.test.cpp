#include <cpp_core/serial.h>
#include <cpp_core/status_code.h>

#include "detail/handle_types.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <string_view>
#include <unistd.h>

#include <gtest/gtest.h>

namespace
{
using cpp_bindings_linux::detail::UniqueFd;
constexpr auto kConfig = cpp_core::SerialConfig::make<9600, cpp_core::DataBits::kEight>();
constexpr auto kTimeout = cpp_core::SerialTimeoutConfig::make<10, 1>();
constexpr int kTimeoutError = static_cast<int>(cpp_core::StatusCode::Configuration::kSetTimeoutError);
constexpr int kBufferError = static_cast<int>(cpp_core::StatusCode::Io::kBufferError);
int g_error = 0;
void captureError(int code, const char *)
{
    g_error = code;
}
void portEvent(cpp_core::PortEvent, const char *)
{
}
} // namespace

TEST(SerialV3ApiTest, RejectsNullAndInvalidOpenConfigurations)
{
    EXPECT_EQ(serialOpen("/dev/null", nullptr), static_cast<int>(cpp_core::StatusCode::Control::kSetStateError));
    const auto check = [](cpp_core::SerialConfig config, int expected) {
        g_error = 0;
        EXPECT_EQ(serialOpen("/dev/null", &config, captureError), expected);
        EXPECT_EQ(g_error, expected);
    };
    auto config = kConfig;
    config.parity = static_cast<cpp_core::Parity>(99);
    check(config, static_cast<int>(cpp_core::StatusCode::Configuration::kSetParityError));
    config = kConfig;
    config.stop_bits = static_cast<cpp_core::StopBits>(1);
    check(config, static_cast<int>(cpp_core::StatusCode::Configuration::kSetStopBitsError));
    config = kConfig;
    config.flow_mode = static_cast<cpp_core::FlowControl>(99);
    check(config, static_cast<int>(cpp_core::StatusCode::Configuration::kSetFlowControlError));
}

TEST(SerialV3ApiTest, RejectsNullNegativeAndOverflowingTimeouts)
{
    UniqueFd fd(open("/dev/null", O_RDWR | O_NONBLOCK));
    ASSERT_TRUE(fd.valid());
    std::array<std::uint8_t, 4> buffer{};
    const auto check = [&](const cpp_core::SerialTimeoutConfig *timeout) {
        g_error = 0;
        EXPECT_EQ(serialRead(fd.get(), buffer.data(), 4, timeout, captureError), kTimeoutError);
        EXPECT_EQ(g_error, kTimeoutError);
        g_error = 0;
        EXPECT_EQ(serialWrite(fd.get(), buffer.data(), 4, timeout, captureError), kTimeoutError);
        EXPECT_EQ(g_error, kTimeoutError);
        EXPECT_EQ(serialReadUntilSequence(fd.get(), buffer.data(), 4, timeout, buffer.data(), 1), kTimeoutError);
    };
    check(nullptr);
    for (const auto timeout : {cpp_core::SerialTimeoutConfig{-1, 1}, {1, -1}, {std::numeric_limits<int>::max(), 2}})
    {
        check(&timeout);
    }
    const cpp_core::SerialTimeoutConfig zero{0, std::numeric_limits<int>::max()};
    EXPECT_EQ(serialRead(fd.get(), buffer.data(), 4, &zero), 0);
    EXPECT_EQ(serialWrite(fd.get(), buffer.data(), 4, &zero), 4);
}

TEST(SerialV3ApiTest, ReadsBinarySequenceWithEmbeddedZeroAndLeavesFollowingBytes)
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

TEST(SerialV3ApiTest, ValidatesSequenceSizeAndHonorsBufferCapacity)
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

TEST(SerialV3ApiTest, OpensPseudoTerminalWithFlowControlAndTypedSettings)
{
    UniqueFd master(posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC));
    ASSERT_TRUE(master.valid());
    ASSERT_EQ(grantpt(master.get()), 0);
    ASSERT_EQ(unlockpt(master.get()), 0);
    const char *path = ptsname(master.get());
    ASSERT_NE(path, nullptr);
    for (auto flow : {cpp_core::FlowControl::kNone, cpp_core::FlowControl::kRtsCts, cpp_core::FlowControl::kXonXoff})
    {
        auto config = kConfig;
        config.flow_mode = flow;
        const auto handle = serialOpen(path, &config);
        ASSERT_GT(handle, 0);
        UniqueFd slave(static_cast<int>(handle));
        EXPECT_EQ(serialGetBaudrate(handle), config.baudrate);
        EXPECT_EQ(serialGetDataBits(handle), config.data_bits);
        EXPECT_EQ(serialGetParity(handle), config.parity);
        EXPECT_EQ(serialGetStopBits(handle), config.stop_bits);
        EXPECT_EQ(serialGetFlowControl(handle), flow);
        EXPECT_EQ(serialSetStopBits(handle, cpp_core::StopBits::kTwo), 0);
        EXPECT_EQ(serialGetStopBits(handle), cpp_core::StopBits::kTwo);
        EXPECT_EQ(serialSetStopBits(handle, static_cast<cpp_core::StopBits>(1)),
                  static_cast<int>(cpp_core::StatusCode::Configuration::kSetStopBitsError));
        EXPECT_EQ(serialSetFlowControl(handle, cpp_core::FlowControl::kNone), 0);
        EXPECT_EQ(serialGetFlowControl(handle), cpp_core::FlowControl::kNone);
        EXPECT_EQ(serialWaitForDrain(handle), 0);
        EXPECT_EQ(serialClose(slave.release()), 0);
    }
}

TEST(SerialV3ApiTest, TypedGettersPreserveNegativeErrors)
{
    constexpr int expected = static_cast<int>(cpp_core::StatusCode::Connection::kInvalidHandleError);
    EXPECT_EQ(cpp_core::toInt(serialGetDataBits(-1)), expected);
    EXPECT_EQ(cpp_core::toInt(serialGetParity(-1)), expected);
    EXPECT_EQ(cpp_core::toInt(serialGetStopBits(-1)), expected);
    EXPECT_EQ(cpp_core::toInt(serialGetFlowControl(-1)), expected);
    EXPECT_EQ(serialWaitForDrain(-1), expected);
}

TEST(SerialV3ApiTest, MetadataDescribesLoadedBinding)
{
    meta(nullptr);
    cpp_core::Meta info{};
    meta(&info);
    EXPECT_STREQ(info.version_string, CPP_BINDINGS_LINUX_TEST_VERSION);
    ASSERT_NE(info.git_commit_hash_full, nullptr);
    EXPECT_EQ(std::strlen(info.git_commit_hash_full), 40U);
    ASSERT_NE(info.git_commit_hash_short, nullptr);
    EXPECT_TRUE(std::string_view(info.git_commit_hash_full).starts_with(info.git_commit_hash_short));
    EXPECT_NE(info.prerelease, nullptr);
    EXPECT_NE(info.prerelease_type, nullptr);
    EXPECT_NE(info.prerelease_number, nullptr);
    EXPECT_NE(info.git_tag, nullptr);
    EXPECT_NE(info.git_commit_date, nullptr);
    EXPECT_NE(info.git_branch, nullptr);
    EXPECT_NE(info.git_dirty_suffix, nullptr);
}

TEST(SerialV3ApiTest, EventCallbackCanBeStartedReplacedAndStoppedRepeatedly)
{
    EXPECT_EQ(serialSetEventCallback(nullptr), 0);
    for (int iteration = 0; iteration < 5; ++iteration)
    {
        EXPECT_EQ(serialSetEventCallback(portEvent), 0);
        EXPECT_EQ(serialSetEventCallback(portEvent), 0);
        EXPECT_EQ(serialSetEventCallback(nullptr), 0);
    }
    EXPECT_EQ(serialSetEventCallback(nullptr), 0);
}

TEST(SerialV3ApiDeathTest, ActiveEventListenerIsStoppedOnExit)
{
    EXPECT_EXIT(
        {
            if (serialSetEventCallback(portEvent) != 0)
            {
                std::exit(1);
            }
            std::exit(0);
        },
        ::testing::ExitedWithCode(0), "");
}
