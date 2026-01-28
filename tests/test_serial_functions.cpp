#include <cpp_core/interface/serial_close.h>
#include <cpp_core/interface/serial_open.h>
#include <cpp_core/interface/serial_read.h>
#include <cpp_core/interface/serial_write.h>
#include <cpp_core/status_codes.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

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
        ErrorCapture::instance = &error_capture_;
        error_callback_ = &ErrorCapture::callback;
    }

    void TearDown() override
    {
        ErrorCapture::instance = nullptr;
    }

    ErrorCapture error_capture_;
    ErrorCallbackT error_callback_ = nullptr;
};

TEST_F(SerialOpenTest, NullPortParameter)
{
    intptr_t result = serialOpen(nullptr, 9600, 8, 0, 1, error_callback_);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kNotFoundError));
    EXPECT_NE(error_capture_.last_message.find("nullptr"), std::string::npos);
}

TEST_F(SerialOpenTest, BaudrateTooLow)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 100, 8, 0, 1, error_callback_);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
    EXPECT_NE(error_capture_.last_message.find("baudrate"), std::string::npos);
}

TEST_F(SerialOpenTest, BaudrateTooLowBoundary)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 299, 8, 0, 1, error_callback_);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, BaudrateBoundaryValid)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 300, 8, 0, 1, error_callback_);

    // /dev/null is not a real serial port, but should pass baudrate validation
    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, DataBitsTooLow)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 4, 0, 1, error_callback_);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
    EXPECT_NE(error_capture_.last_message.find("data bits"), std::string::npos);
}

TEST_F(SerialOpenTest, DataBitsTooHigh)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 9, 0, 1, error_callback_);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidDataBits5)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 5, 0, 1, error_callback_);

    // Should pass data bits validation
    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidDataBits6)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 6, 0, 1, error_callback_);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidDataBits7)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 7, 0, 1, error_callback_);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidDataBits8)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 1, error_callback_);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, InvalidParity)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 5, 1, error_callback_);

    // Invalid parity should return an error
    EXPECT_LT(result, 0);
}

TEST_F(SerialOpenTest, ValidParityNone)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 1, error_callback_);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidParityEven)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 1, 1, error_callback_);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidParityOdd)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 2, 1, error_callback_);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, InvalidStopBits)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 3, error_callback_);

    // Invalid stop bits should return an error
    EXPECT_LT(result, 0);
}

TEST_F(SerialOpenTest, ValidStopBits0)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 0, error_callback_);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidStopBits1)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 1, error_callback_);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidStopBits2)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 2, error_callback_);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, NonExistentPort)
{
    const char *port = "/dev/ttyNONEXISTENT99999";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 1, error_callback_);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kNotFoundError));
}

TEST_F(SerialOpenTest, VariousBaudrates)
{
    const char *port = "/dev/null";

    // Test common baudrates
    int baudrates[] = {300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800};

    for (int baudrate : baudrates)
    {
        intptr_t result =
            serialOpen(const_cast<void *>(static_cast<const void *>(port)), baudrate, 8, 0, 1, error_callback_);
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
        ErrorCapture::instance = &error_capture_;
        error_callback_ = &ErrorCapture::callback;
    }

    void TearDown() override
    {
        ErrorCapture::instance = nullptr;
    }

    ErrorCapture error_capture_;
    ErrorCallbackT error_callback_ = nullptr;
};

TEST_F(SerialCloseTest, CloseInvalidHandleZero)
{
    int result = serialClose(0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kSuccess));
}

TEST_F(SerialCloseTest, CloseInvalidHandleNegative)
{
    int result = serialClose(-1, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kSuccess));
}

TEST_F(SerialCloseTest, CloseInvalidHandleNegativeLarge)
{
    int result = serialClose(-12345, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kSuccess));
}

TEST_F(SerialCloseTest, CloseInvalidHandleTooLarge)
{
    int64_t too_large_handle = static_cast<int64_t>(std::numeric_limits<int>::max()) + 1;
    int result = serialClose(too_large_handle, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialCloseTest, CloseInvalidHandleIntMaxBoundary)
{
    int64_t handle = static_cast<int64_t>(std::numeric_limits<int>::max());
    int result = serialClose(handle, error_callback_);

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
    int result = serialClose(9999, error_callback_);

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
        ErrorCapture::instance = &error_capture_;
        error_callback_ = &ErrorCapture::callback;
    }

    void TearDown() override
    {
        ErrorCapture::instance = nullptr;
    }

    ErrorCapture error_capture_;
    ErrorCallbackT error_callback_ = nullptr;
};

TEST_F(SerialReadTest, ReadNullBuffer)
{
    int result = serialRead(1, nullptr, 10, 100, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kBufferError));
    EXPECT_NE(error_capture_.last_message.find("buffer"), std::string::npos);
}

TEST_F(SerialReadTest, ReadZeroBufferSize)
{
    char buffer[10] = {0};
    int result = serialRead(1, buffer, 0, 100, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kBufferError));
}

TEST_F(SerialReadTest, ReadNegativeBufferSize)
{
    char buffer[10] = {0};
    int result = serialRead(1, buffer, -1, 100, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kBufferError));
}

TEST_F(SerialReadTest, ReadInvalidHandleZero)
{
    char buffer[10] = {0};
    int result = serialRead(0, buffer, sizeof(buffer), 100, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialReadTest, ReadInvalidHandleNegative)
{
    char buffer[10] = {0};
    int result = serialRead(-1, buffer, sizeof(buffer), 100, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialReadTest, ReadInvalidHandleTooLarge)
{
    char buffer[10] = {0};
    int64_t too_large = static_cast<int64_t>(std::numeric_limits<int>::max()) + 1;
    int result = serialRead(too_large, buffer, sizeof(buffer), 100, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialReadTest, ReadFromDevNull)
{
    int fd = open("/dev/null", O_RDONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    char buffer[10] = {0};
    int result = serialRead(fd, buffer, sizeof(buffer), 0, 0, error_callback_);

    EXPECT_EQ(result, 0);
    close(fd);
}

TEST_F(SerialReadTest, ReadWithLargeBufferSize)
{
    int fd = open("/dev/null", O_RDONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    char buffer[4096] = {0};
    int result = serialRead(fd, buffer, sizeof(buffer), 0, 0, error_callback_);

    EXPECT_EQ(result, 0);
    close(fd);
}

TEST_F(SerialReadTest, ReadNoErrorCallback)
{
    char buffer[10] = {0};
    int result = serialRead(0, buffer, sizeof(buffer), 100, 0, nullptr);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialReadTest, ReadWithVariousTimeouts)
{
    int fd = open("/dev/null", O_RDONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    char buffer[10] = {0};

    // Test various timeout values
    for (int timeout : {0, 1, 10, 100, 1000})
    {
        int result = serialRead(fd, buffer, sizeof(buffer), timeout, 0, error_callback_);
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
        ErrorCapture::instance = &error_capture_;
        error_callback_ = &ErrorCapture::callback;
    }

    void TearDown() override
    {
        ErrorCapture::instance = nullptr;
    }

    ErrorCapture error_capture_;
    ErrorCallbackT error_callback_ = nullptr;
};

TEST_F(SerialWriteTest, WriteNullBuffer)
{
    int result = serialWrite(1, nullptr, 10, 100, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kBufferError));
    EXPECT_NE(error_capture_.last_message.find("buffer"), std::string::npos);
}

TEST_F(SerialWriteTest, WriteZeroBufferSize)
{
    char buffer[10] = {0};
    int result = serialWrite(1, buffer, 0, 100, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kBufferError));
}

TEST_F(SerialWriteTest, WriteNegativeBufferSize)
{
    char buffer[10] = {0};
    int result = serialWrite(1, buffer, -1, 100, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kBufferError));
}

TEST_F(SerialWriteTest, WriteInvalidHandleZero)
{
    const char *buffer = "test";
    int result = serialWrite(0, buffer, strlen(buffer), 100, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialWriteTest, WriteInvalidHandleNegative)
{
    const char *buffer = "test";
    int result = serialWrite(-1, buffer, strlen(buffer), 100, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialWriteTest, WriteInvalidHandleTooLarge)
{
    const char *buffer = "test";
    int64_t too_large = static_cast<int64_t>(std::numeric_limits<int>::max()) + 1;
    int result = serialWrite(too_large, buffer, strlen(buffer), 100, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError));
}

TEST_F(SerialWriteTest, WriteToDevNull)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    const char *test_data = "Hello World";
    int result = serialWrite(fd, test_data, strlen(test_data), 0, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(strlen(test_data)));
    close(fd);
}

TEST_F(SerialWriteTest, WriteLargeBuffer)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    std::string large_data(4096, 'A');
    int result = serialWrite(fd, large_data.c_str(), large_data.size(), 0, 0, error_callback_);

    EXPECT_EQ(result, static_cast<int>(large_data.size()));
    close(fd);
}

TEST_F(SerialWriteTest, WriteMultipleSmallBuffers)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    for (int i = 0; i < 10; ++i)
    {
        const char *data = "test";
        int result = serialWrite(fd, data, strlen(data), 0, 0, error_callback_);
        EXPECT_EQ(result, static_cast<int>(strlen(data)));
    }

    close(fd);
}

TEST_F(SerialWriteTest, WriteNoErrorCallback)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    const char *test_data = "test";
    int result = serialWrite(fd, test_data, strlen(test_data), 0, 0, nullptr);

    EXPECT_EQ(result, static_cast<int>(strlen(test_data)));
    close(fd);
}

TEST_F(SerialWriteTest, WriteWithVariousTimeouts)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    const char *test_data = "test";

    // Test various timeout values
    for (int timeout : {0, 1, 10, 100, 1000})
    {
        int result = serialWrite(fd, test_data, strlen(test_data), timeout, 0, error_callback_);
        EXPECT_EQ(result, static_cast<int>(strlen(test_data)))
            << "Timeout " << timeout << " should succeed for /dev/null";
    }

    close(fd);
}

TEST_F(SerialWriteTest, WriteEmptyStringToDevNull)
{
    int fd = open("/dev/null", O_WRONLY | O_NONBLOCK);
    ASSERT_GE(fd, 0);

    const char *empty = "";
    // This should fail because buffer_size is 0
    int result = serialWrite(fd, empty, 0, 0, 0, error_callback_);

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
        ErrorCapture::instance = &error_capture_;
        error_callback_ = &ErrorCapture::callback;
    }

    void TearDown() override
    {
        ErrorCapture::instance = nullptr;
    }

    ErrorCapture error_capture_;
    ErrorCallbackT error_callback_ = nullptr;
};

TEST_F(SerialIntegrationTest, ReadWritePipeRoundTrip)
{
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    // Set non-blocking
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

    const char *test_message = "Hello";
    int write_result = serialWrite(pipefd[1], test_message, strlen(test_message), 100, 0, error_callback_);
    EXPECT_EQ(write_result, static_cast<int>(strlen(test_message)));

    char read_buffer[10] = {0};
    int read_result = serialRead(pipefd[0], read_buffer, sizeof(read_buffer), 100, 0, error_callback_);
    EXPECT_EQ(read_result, static_cast<int>(strlen(test_message)));
    EXPECT_EQ(std::string(read_buffer), std::string(test_message));

    close(pipefd[0]);
    close(pipefd[1]);
}

TEST_F(SerialIntegrationTest, MultipleWrites)
{
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

    const char *msg1 = "Hello";
    const char *msg2 = "World";

    serialWrite(pipefd[1], msg1, strlen(msg1), 100, 0, error_callback_);
    serialWrite(pipefd[1], msg2, strlen(msg2), 100, 0, error_callback_);

    char read_buffer[20] = {0};
    int read_result = serialRead(pipefd[0], read_buffer, sizeof(read_buffer), 100, 0, error_callback_);
    EXPECT_GE(read_result, 0);

    close(pipefd[0]);
    close(pipefd[1]);
}

TEST_F(SerialIntegrationTest, CloseAfterOperations)
{
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    const char *test_data = "test";
    serialWrite(pipefd[1], test_data, strlen(test_data), 100, 0, error_callback_);

    char buffer[10] = {0};
    serialRead(pipefd[0], buffer, sizeof(buffer), 100, 0, error_callback_);

    int close_result1 = serialClose(pipefd[0], error_callback_);
    int close_result2 = serialClose(pipefd[1], error_callback_);

    EXPECT_EQ(close_result1, static_cast<int>(cpp_core::StatusCodes::kSuccess));
    EXPECT_EQ(close_result2, static_cast<int>(cpp_core::StatusCodes::kSuccess));
}
