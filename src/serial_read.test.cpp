#include <cpp_core/interface/serial_read.h>
#include <cpp_core/status_code.h>

#include "detail/handle_types.hpp"

#include <array>
#include <fcntl.h>
#include <limits>
#include <unistd.h>

#include <gtest/gtest.h>

#include "test_helpers/error_capture.hpp"

namespace
{
constexpr auto kBufferError = static_cast<int>(cpp_core::StatusCode::Io::kBufferError);
constexpr auto kInvalidHandleError = static_cast<int>(cpp_core::StatusCode::Connection::kInvalidHandleError);
} // namespace

class SerialReadTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ErrorCapture::instance = &error_capture;
        error_callback = &ErrorCapture::callback;
    }

    void TearDown() override
    {
        ErrorCapture::instance = nullptr;
    }

    ErrorCapture error_capture;
    ErrorCallbackT error_callback = nullptr;
};

TEST_F(SerialReadTest, ReadNullBuffer)
{
    const cpp_core::SerialTimeoutConfig timeout_config{100, 0};
    int result = serialRead(1, nullptr, 10, &timeout_config, error_callback);

    EXPECT_EQ(result, kBufferError);
    EXPECT_NE(error_capture.last_message.find("buffer"), std::string::npos);
}

TEST_F(SerialReadTest, ReadZeroBufferSize)
{
    std::array<char, 10> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config{100, 0};
    int result = serialRead(1, reinterpret_cast<std::uint8_t *>(buffer.data()), 0, &timeout_config, error_callback);

    EXPECT_EQ(result, kBufferError);
}

TEST_F(SerialReadTest, ReadNegativeBufferSize)
{
    std::array<char, 10> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config{100, 0};
    int result = serialRead(1, reinterpret_cast<std::uint8_t *>(buffer.data()), -1, &timeout_config, error_callback);

    EXPECT_EQ(result, kBufferError);
}

TEST_F(SerialReadTest, ReadInvalidHandleZero)
{
    std::array<char, 10> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config{100, 0};
    int result = serialRead(0, reinterpret_cast<std::uint8_t *>(buffer.data()), static_cast<int>(buffer.size()),
                            &timeout_config, error_callback);

    EXPECT_EQ(result, kInvalidHandleError);
}

TEST_F(SerialReadTest, ReadInvalidHandleNegative)
{
    std::array<char, 10> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config{100, 0};
    int result = serialRead(-1, reinterpret_cast<std::uint8_t *>(buffer.data()), static_cast<int>(buffer.size()),
                            &timeout_config, error_callback);

    EXPECT_EQ(result, kInvalidHandleError);
}

TEST_F(SerialReadTest, ReadInvalidHandleTooLarge)
{
    std::array<char, 10> buffer{};
    auto too_large = static_cast<int64_t>(std::numeric_limits<int>::max()) + 1;
    const cpp_core::SerialTimeoutConfig timeout_config{100, 0};
    int result = serialRead(too_large, reinterpret_cast<std::uint8_t *>(buffer.data()), static_cast<int>(buffer.size()),
                            &timeout_config, error_callback);

    EXPECT_EQ(result, kInvalidHandleError);
}

TEST_F(SerialReadTest, ReadFromDevNull)
{
    int fd = open("/dev/null", O_RDONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    std::array<char, 10> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config{0, 0};
    int result = serialRead(fd, reinterpret_cast<std::uint8_t *>(buffer.data()), static_cast<int>(buffer.size()),
                            &timeout_config, error_callback);

    EXPECT_EQ(result, 0);
    close(fd);
}

TEST_F(SerialReadTest, ReadWithLargeBufferSize)
{
    int fd = open("/dev/null", O_RDONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    std::array<char, 4096> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config{0, 0};
    int result = serialRead(fd, reinterpret_cast<std::uint8_t *>(buffer.data()), static_cast<int>(buffer.size()),
                            &timeout_config, error_callback);

    EXPECT_EQ(result, 0);
    close(fd);
}

TEST_F(SerialReadTest, ReadNoErrorCallback)
{
    std::array<char, 10> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config{100, 0};
    int result = serialRead(0, reinterpret_cast<std::uint8_t *>(buffer.data()), static_cast<int>(buffer.size()),
                            &timeout_config, nullptr);

    EXPECT_EQ(result, kInvalidHandleError);
}

TEST_F(SerialReadTest, ReadWithVariousTimeouts)
{
    int fd = open("/dev/null", O_RDONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    std::array<char, 10> buffer{};

    for (int timeout : {0, 1, 10, 100, 1000})
    {
        const cpp_core::SerialTimeoutConfig timeout_config{timeout, 0};
        int result = serialRead(fd, reinterpret_cast<std::uint8_t *>(buffer.data()), static_cast<int>(buffer.size()),
                                &timeout_config, error_callback);
        EXPECT_EQ(result, 0) << "Timeout " << timeout << " should return 0 for /dev/null";
    }

    close(fd);
}

TEST_F(SerialReadTest, RejectsNullNegativeAndOverflowingTimeouts)
{
    constexpr int kTimeoutError = static_cast<int>(cpp_core::StatusCode::Configuration::kSetTimeoutError);
    cpp_bindings_linux::detail::UniqueFd fd(open("/dev/null", O_RDWR | O_NONBLOCK));
    ASSERT_TRUE(fd.valid());
    std::array<std::uint8_t, 4> buffer{};
    const auto check = [&](const cpp_core::SerialTimeoutConfig *timeout) {
        error_capture.last_code = 0;
        EXPECT_EQ(serialRead(fd.get(), buffer.data(), 4, timeout, error_callback), kTimeoutError);
        EXPECT_EQ(error_capture.last_code, kTimeoutError);
    };
    check(nullptr);
    for (const auto timeout : {cpp_core::SerialTimeoutConfig{-1, 1}, {1, -1}, {std::numeric_limits<int>::max(), 2}})
    {
        check(&timeout);
    }
}

TEST_F(SerialReadTest, AcceptsZeroTimeoutWithMaximumMultiplier)
{
    cpp_bindings_linux::detail::UniqueFd fd(open("/dev/null", O_RDWR | O_NONBLOCK));
    ASSERT_TRUE(fd.valid());
    std::array<std::uint8_t, 4> buffer{};
    const cpp_core::SerialTimeoutConfig zero{0, std::numeric_limits<int>::max()};
    EXPECT_EQ(serialRead(fd.get(), buffer.data(), 4, &zero), 0);
}
