#include <cpp_core/interface/serial_write.h>
#include <cpp_core/status_code.h>

#include <array>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <string>
#include <unistd.h>

#include <gtest/gtest.h>

#include "test_helpers/error_capture.hpp"

namespace
{
constexpr auto kBufferError = static_cast<int>(cpp_core::StatusCode::Io::kBufferError);
constexpr auto kInvalidHandleError = static_cast<int>(cpp_core::StatusCode::Connection::kInvalidHandleError);
} // namespace

class SerialWriteTest : public ::testing::Test
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

TEST_F(SerialWriteTest, WriteNullBuffer)
{
    const cpp_core::SerialTimeoutConfig timeout_config{100, 0};
    int result = serialWrite(1, nullptr, 10, &timeout_config, error_callback);

    EXPECT_EQ(result, kBufferError);
    EXPECT_NE(error_capture.last_message.find("buffer"), std::string::npos);
}

TEST_F(SerialWriteTest, WriteZeroBufferSize)
{
    std::array<char, 10> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config{100, 0};
    int result =
        serialWrite(1, reinterpret_cast<const std::uint8_t *>(buffer.data()), 0, &timeout_config, error_callback);

    EXPECT_EQ(result, kBufferError);
}

TEST_F(SerialWriteTest, WriteNegativeBufferSize)
{
    std::array<char, 10> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config{100, 0};
    int result =
        serialWrite(1, reinterpret_cast<const std::uint8_t *>(buffer.data()), -1, &timeout_config, error_callback);

    EXPECT_EQ(result, kBufferError);
}

TEST_F(SerialWriteTest, WriteInvalidHandleZero)
{
    const char *buffer = "test";
    const cpp_core::SerialTimeoutConfig timeout_config{100, 0};
    int result = serialWrite(0, reinterpret_cast<const std::uint8_t *>(buffer), static_cast<int>(strlen(buffer)),
                             &timeout_config, error_callback);

    EXPECT_EQ(result, kInvalidHandleError);
}

TEST_F(SerialWriteTest, WriteInvalidHandleNegative)
{
    const char *buffer = "test";
    const cpp_core::SerialTimeoutConfig timeout_config{100, 0};
    int result = serialWrite(-1, reinterpret_cast<const std::uint8_t *>(buffer), static_cast<int>(strlen(buffer)),
                             &timeout_config, error_callback);

    EXPECT_EQ(result, kInvalidHandleError);
}

TEST_F(SerialWriteTest, WriteInvalidHandleTooLarge)
{
    const char *buffer = "test";
    auto too_large = static_cast<int64_t>(std::numeric_limits<int>::max()) + 1;
    const cpp_core::SerialTimeoutConfig timeout_config{100, 0};
    int result = serialWrite(too_large, reinterpret_cast<const std::uint8_t *>(buffer),
                             static_cast<int>(strlen(buffer)), &timeout_config, error_callback);

    EXPECT_EQ(result, kInvalidHandleError);
}

TEST_F(SerialWriteTest, WriteToDevNull)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    const char *test_data = "Hello World";
    const int len = static_cast<int>(strlen(test_data));
    const cpp_core::SerialTimeoutConfig timeout_config{0, 0};
    int result =
        serialWrite(fd, reinterpret_cast<const std::uint8_t *>(test_data), len, &timeout_config, error_callback);

    EXPECT_EQ(result, len);
    close(fd);
}

TEST_F(SerialWriteTest, WriteLargeBuffer)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    std::string large_data(4096, 'A');
    const int len = static_cast<int>(large_data.size());
    const cpp_core::SerialTimeoutConfig timeout_config{0, 0};
    int result = serialWrite(fd, reinterpret_cast<const std::uint8_t *>(large_data.c_str()), len, &timeout_config,
                             error_callback);

    EXPECT_EQ(result, len);
    close(fd);
}

TEST_F(SerialWriteTest, WriteMultipleSmallBuffers)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    const char *data = "test";
    const int len = static_cast<int>(strlen(data));
    for (int i = 0; i < 10; ++i)
    {
        const cpp_core::SerialTimeoutConfig timeout_config{0, 0};
        int result =
            serialWrite(fd, reinterpret_cast<const std::uint8_t *>(data), len, &timeout_config, error_callback);
        EXPECT_EQ(result, len);
    }

    close(fd);
}

TEST_F(SerialWriteTest, WriteNoErrorCallback)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    const char *test_data = "test";
    const int len = static_cast<int>(strlen(test_data));
    const cpp_core::SerialTimeoutConfig timeout_config{0, 0};
    int result = serialWrite(fd, reinterpret_cast<const std::uint8_t *>(test_data), len, &timeout_config, nullptr);

    EXPECT_EQ(result, len);
    close(fd);
}

TEST_F(SerialWriteTest, WriteWithVariousTimeouts)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    const char *test_data = "test";
    const int len = static_cast<int>(strlen(test_data));

    for (int timeout : {0, 1, 10, 100, 1000})
    {
        const cpp_core::SerialTimeoutConfig timeout_config{timeout, 0};
        int result =
            serialWrite(fd, reinterpret_cast<const std::uint8_t *>(test_data), len, &timeout_config, error_callback);
        EXPECT_EQ(result, len) << "Timeout " << timeout << " should succeed for /dev/null";
    }

    close(fd);
}

TEST_F(SerialWriteTest, WriteEmptyStringToDevNull)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    const char *empty = "";
    const cpp_core::SerialTimeoutConfig timeout_config{0, 0};
    int result = serialWrite(fd, reinterpret_cast<const std::uint8_t *>(empty), 0, &timeout_config, error_callback);

    EXPECT_EQ(result, kBufferError);
    close(fd);
}
