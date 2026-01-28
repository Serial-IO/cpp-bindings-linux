#include <cpp_core/interface/serial_close.h>
#include <cpp_core/interface/serial_open.h>
#include <cpp_core/interface/serial_read.h>
#include <cpp_core/interface/serial_write.h>
#include <cpp_core/status_codes.h>

#include <array>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Helper to capture error callback
struct ErrorCapture
{
    int last_code = 0;
    std::string last_message;

    static void callback(int code, const char *message)
    {
        if (instance != nullptr)
        {
            instance->last_code = code;
            instance->last_message = message != nullptr ? message : "";
        }
    }

    static ErrorCapture *instance;
};

ErrorCapture *ErrorCapture::instance = nullptr;

// ============================================================================
// Tests for serialOpen
// ============================================================================

class SerialOpenTest : public ::testing::Test
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

TEST_F(SerialOpenTest, NullPortParameter)
{
    intptr_t result = serialOpen(nullptr, 9600, 8, 0, 1, error_callback);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kNotFoundError));
    EXPECT_NE(error_capture.last_message.find("nullptr"), std::string::npos);
}

TEST_F(SerialOpenTest, BaudrateTooLow)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 100, 8, 0, 1, error_callback);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
    EXPECT_NE(error_capture.last_message.find("baudrate"), std::string::npos);
}

TEST_F(SerialOpenTest, BaudrateTooLowBoundary)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 299, 8, 0, 1, error_callback);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, BaudrateBoundaryValid)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 300, 8, 0, 1, error_callback);

    // /dev/null is not a real serial port, but should pass baudrate validation
    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, DataBitsTooLow)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 4, 0, 1, error_callback);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
    EXPECT_NE(error_capture.last_message.find("data bits"), std::string::npos);
}

TEST_F(SerialOpenTest, DataBitsTooHigh)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 9, 0, 1, error_callback);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidDataBits5)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 5, 0, 1, error_callback);

    // Should pass data bits validation
    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidDataBits6)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 6, 0, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidDataBits7)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 7, 0, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidDataBits8)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, InvalidParity)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 5, 1, error_callback);

    // Invalid parity should return an error
    EXPECT_LT(result, 0);
}

TEST_F(SerialOpenTest, ValidParityNone)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidParityEven)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 1, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidParityOdd)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 2, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, InvalidStopBits)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 3, error_callback);

    // Invalid stop bits should return an error
    EXPECT_LT(result, 0);
}

TEST_F(SerialOpenTest, ValidStopBits0)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 0, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidStopBits1)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidStopBits2)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 2, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, NonExistentPort)
{
    const char *port = "/dev/ttyNONEXISTENT99999";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 1, error_callback);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kNotFoundError));
}

TEST_F(SerialOpenTest, VariousBaudrates)
{
    const char *port = "/dev/null";

    // Test common baudrates
    const std::array<int, 11> baudrates = {300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800};

    for (int baudrate : baudrates)
    {
        intptr_t result =
            serialOpen(const_cast<void *>(static_cast<const void *>(port)), baudrate, 8, 0, 1, error_callback);
        EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError))
            << "Baudrate " << baudrate << " should be valid";
    }
}

TEST_F(SerialOpenTest, NoErrorCallbackNullPort)
{
    intptr_t result = serialOpen(nullptr, 9600, 8, 0, 1, nullptr);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kNotFoundError));
}

// ============================================================================
// Tests for serialClose
// ============================================================================

class SerialCloseTest : public ::testing::Test
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

TEST_F(SerialCloseTest, CloseInvalidHandleZero)
{
    int result = serialClose(0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kSuccess));
}

TEST_F(SerialCloseTest, CloseInvalidHandleNegative)
{
    int result = serialClose(-1, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kSuccess));
}

TEST_F(SerialCloseTest, CloseInvalidHandleNegativeLarge)
{
    int result = serialClose(-12345, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kSuccess));
}

TEST_F(SerialCloseTest, CloseInvalidHandleTooLarge)
{
    auto too_large_handle = static_cast<int64_t>(std::numeric_limits<int>::max()) + 1;
    int result = serialClose(too_large_handle, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialCloseTest, CloseInvalidHandleIntMaxBoundary)
{
    auto handle = static_cast<int64_t>(std::numeric_limits<int>::max());
    int result = serialClose(handle, error_callback);

    // Should fail because this fd doesn't exist, but not with InvalidHandleError
    EXPECT_NE(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialCloseTest, CloseNoErrorCallback)
{
    int result = serialClose(0, nullptr);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kSuccess));
}

TEST_F(SerialCloseTest, CloseInvalidHandle)
{
    // Test closing a real invalid fd (one that never existed)
    // We should get an error but not crash
    int result = serialClose(9999, error_callback);

    // This will fail because the fd doesn't exist
    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kCloseHandleError));
}

// ============================================================================
// Tests for serialRead
// ============================================================================

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
    int result = serialRead(1, nullptr, 10, 100, 0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kBufferError));
    EXPECT_NE(error_capture.last_message.find("buffer"), std::string::npos);
}

TEST_F(SerialReadTest, ReadZeroBufferSize)
{
    std::array<char, 10> buffer{};
    int result = serialRead(1, buffer.data(), 0, 100, 0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kBufferError));
}

TEST_F(SerialReadTest, ReadNegativeBufferSize)
{
    std::array<char, 10> buffer{};
    int result = serialRead(1, buffer.data(), -1, 100, 0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kBufferError));
}

TEST_F(SerialReadTest, ReadInvalidHandleZero)
{
    std::array<char, 10> buffer{};
    int result = serialRead(0, buffer.data(), static_cast<int>(buffer.size()), 100, 0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialReadTest, ReadInvalidHandleNegative)
{
    std::array<char, 10> buffer{};
    int result = serialRead(-1, buffer.data(), static_cast<int>(buffer.size()), 100, 0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialReadTest, ReadInvalidHandleTooLarge)
{
    std::array<char, 10> buffer{};
    auto too_large = static_cast<int64_t>(std::numeric_limits<int>::max()) + 1;
    int result = serialRead(too_large, buffer.data(), static_cast<int>(buffer.size()), 100, 0, error_callback);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialReadTest, ReadFromDevNull)
{
    int fd = open("/dev/null", O_RDONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    std::array<char, 10> buffer{};
    int result = serialRead(fd, buffer.data(), static_cast<int>(buffer.size()), 0, 0, error_callback);

    EXPECT_EQ(result, 0);
    close(fd);
}

TEST_F(SerialReadTest, ReadWithLargeBufferSize)
{
    int fd = open("/dev/null", O_RDONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    std::array<char, 4096> buffer{};
    int result = serialRead(fd, buffer.data(), static_cast<int>(buffer.size()), 0, 0, error_callback);

    EXPECT_EQ(result, 0);
    close(fd);
}

TEST_F(SerialReadTest, ReadNoErrorCallback)
{
    std::array<char, 10> buffer{};
    int result = serialRead(0, buffer.data(), static_cast<int>(buffer.size()), 100, 0, nullptr);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialReadTest, ReadWithVariousTimeouts)
{
    int fd = open("/dev/null", O_RDONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    std::array<char, 10> buffer{};

    // Test various timeout values
    for (int timeout : {0, 1, 10, 100, 1000})
    {
        int result = serialRead(fd, buffer.data(), static_cast<int>(buffer.size()), timeout, 0, error_callback);
        EXPECT_EQ(result, 0) << "Timeout " << timeout << " should return 0 for /dev/null";
    }

    close(fd);
}

// ============================================================================
// Tests for serialWrite
// ============================================================================

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

// ============================================================================
// Integration Tests
// ============================================================================

class SerialIntegrationTest : public ::testing::Test
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

TEST_F(SerialIntegrationTest, ReadWritePipeRoundTrip)
{
    std::array<int, 2> pipefd{};
    ASSERT_EQ(pipe(pipefd.data()), 0);

    // Set non-blocking
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

    const char *test_message = "Hello";
    const int msg_len = static_cast<int>(strlen(test_message));
    int write_result = serialWrite(pipefd[1], test_message, msg_len, 100, 0, error_callback);
    EXPECT_EQ(write_result, msg_len);

    std::array<char, 10> read_buffer{};
    int read_result =
        serialRead(pipefd[0], read_buffer.data(), static_cast<int>(read_buffer.size()), 100, 0, error_callback);
    EXPECT_EQ(read_result, msg_len);
    EXPECT_EQ(std::string(read_buffer.data()), std::string(test_message));

    close(pipefd[0]);
    close(pipefd[1]);
}

TEST_F(SerialIntegrationTest, MultipleWrites)
{
    std::array<int, 2> pipefd{};
    ASSERT_EQ(pipe(pipefd.data()), 0);

    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

    const char *msg1 = "Hello";
    const char *msg2 = "World";

    serialWrite(pipefd[1], msg1, static_cast<int>(strlen(msg1)), 100, 0, error_callback);
    serialWrite(pipefd[1], msg2, static_cast<int>(strlen(msg2)), 100, 0, error_callback);

    std::array<char, 20> read_buffer{};
    int read_result =
        serialRead(pipefd[0], read_buffer.data(), static_cast<int>(read_buffer.size()), 100, 0, error_callback);
    EXPECT_GE(read_result, 0);

    close(pipefd[0]);
    close(pipefd[1]);
}

TEST_F(SerialIntegrationTest, CloseAfterOperations)
{
    std::array<int, 2> pipefd{};
    ASSERT_EQ(pipe(pipefd.data()), 0);

    const char *test_data = "test";
    serialWrite(pipefd[1], test_data, static_cast<int>(strlen(test_data)), 100, 0, error_callback);

    std::array<char, 10> buffer{};
    serialRead(pipefd[0], buffer.data(), static_cast<int>(buffer.size()), 100, 0, error_callback);

    int close_result1 = serialClose(pipefd[0], error_callback);
    int close_result2 = serialClose(pipefd[1], error_callback);

    EXPECT_EQ(close_result1, static_cast<int>(cpp_core::StatusCodes::kSuccess));
    EXPECT_EQ(close_result2, static_cast<int>(cpp_core::StatusCodes::kSuccess));
}
