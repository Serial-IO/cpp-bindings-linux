#include <cpp_core/interface/serial_close.h>
#include <cpp_core/interface/serial_open.h>
#include <cpp_core/interface/serial_read.h>
#include <cpp_core/interface/serial_write.h>
#include <cpp_core/status_codes.h>

#include <array>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <unistd.h>

#include <gtest/gtest.h>

#include "test_helpers/error_capture.hpp"

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
