#include <cpp_core/interface/serial_write.h>
#include <cpp_core/status_codes.h>

#include <array>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <string>
#include <unistd.h>

#include <gtest/gtest.h>

#include "test_helpers/error_capture.hpp"

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
    int result = serialWrite(1, nullptr, 10, 100, 0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kBufferError));
    EXPECT_NE(error_capture.last_message.find("buffer"), std::string::npos);
}

TEST_F(SerialWriteTest, WriteZeroBufferSize)
{
    std::array<char, 10> buffer{};
    int result = serialWrite(1, buffer.data(), 0, 100, 0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kBufferError));
}

TEST_F(SerialWriteTest, WriteNegativeBufferSize)
{
    std::array<char, 10> buffer{};
    int result = serialWrite(1, buffer.data(), -1, 100, 0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kBufferError));
}

TEST_F(SerialWriteTest, WriteInvalidHandleZero)
{
    const char *buffer = "test";
    int result = serialWrite(0, buffer, static_cast<int>(strlen(buffer)), 100, 0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialWriteTest, WriteInvalidHandleNegative)
{
    const char *buffer = "test";
    int result = serialWrite(-1, buffer, static_cast<int>(strlen(buffer)), 100, 0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialWriteTest, WriteInvalidHandleTooLarge)
{
    const char *buffer = "test";
    auto too_large = static_cast<int64_t>(std::numeric_limits<int>::max()) + 1;
    int result = serialWrite(too_large, buffer, static_cast<int>(strlen(buffer)), 100, 0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialWriteTest, WriteToDevNull)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    const char *test_data = "Hello World";
    const int len = static_cast<int>(strlen(test_data));
    int result = serialWrite(fd, test_data, len, 0, 0, error_callback);

    EXPECT_EQ(result, len);
    close(fd);
}

TEST_F(SerialWriteTest, WriteLargeBuffer)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    std::string large_data(4096, 'A');
    const int len = static_cast<int>(large_data.size());
    int result = serialWrite(fd, large_data.c_str(), len, 0, 0, error_callback);

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
        int result = serialWrite(fd, data, len, 0, 0, error_callback);
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
    int result = serialWrite(fd, test_data, len, 0, 0, nullptr);

    EXPECT_EQ(result, len);
    close(fd);
}

TEST_F(SerialWriteTest, WriteWithVariousTimeouts)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    const char *test_data = "test";
    const int len = static_cast<int>(strlen(test_data));

    // Test various timeout values
    for (int timeout : {0, 1, 10, 100, 1000})
    {
        int result = serialWrite(fd, test_data, len, timeout, 0, error_callback);
        EXPECT_EQ(result, len) << "Timeout " << timeout << " should succeed for /dev/null";
    }

    close(fd);
}

TEST_F(SerialWriteTest, WriteEmptyStringToDevNull)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    const char *empty = "";
    // This should fail because buffer_size is 0
    int result = serialWrite(fd, empty, 0, 0, 0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kBufferError));
    close(fd);
}
