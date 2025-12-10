// Test for serial communication with Arduino echo script on /dev/ttyUSB0

#include <cpp_core/interface/serial_close.h>
#include <cpp_core/interface/serial_open.h>
#include <cpp_core/interface/serial_read.h>
#include <cpp_core/interface/serial_write.h>
#include <cpp_core/serial.h>
#include <cpp_core/status_codes.h>
#include <gtest/gtest.h>

#include <cstring>

class SerialArduinoTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const char *port = "/dev/ttyUSB0";
        handle_ = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 115200, 8, 0, 0, nullptr);

        if (handle_ <= 0)
        {
            GTEST_SKIP() << "Could not open /dev/ttyUSB0. "
                            "Make sure Arduino is connected and accessible.";
        }

        // Wait for device to initialize after opening (e.g., Arduino reset)
        usleep(2000000); // 2 seconds
    }

    void TearDown() override
    {
        if (handle_ > 0)
        {
            serialClose(handle_, nullptr);
            handle_ = 0;
        }
    }

    intptr_t handle_ = 0;
};

// Test basic open/close
TEST_F(SerialArduinoTest, OpenClose)
{
    EXPECT_GT(handle_, 0) << "serialOpen should return a positive handle";
}

// Test write and read echo
TEST_F(SerialArduinoTest, WriteReadEcho)
{
    const char *test_message = "Hello Arduino!\n";
    int message_len = static_cast<int>(strlen(test_message));

    // Write message
    int written = serialWrite(handle_, test_message, message_len, 1000, 1, nullptr);
    EXPECT_EQ(written, message_len) << "Should write all bytes. Written: " << written << ", Expected: " << message_len;

    // Delay to allow Arduino to process and echo
    usleep(500000); // 500ms - give Arduino more time

    // Read echo back
    char read_buffer[256] = {0};
    int read_bytes = serialRead(handle_, read_buffer, sizeof(read_buffer) - 1, 2000, 1, nullptr);

    EXPECT_GT(read_bytes, 0) << "Should read at least some bytes";
    EXPECT_LE(read_bytes, static_cast<int>(sizeof(read_buffer) - 1)) << "Should not overflow buffer";

    // Check if we got the echo
    read_buffer[read_bytes] = '\0';
    EXPECT_STRNE(read_buffer, "") << "Should receive echo from Arduino";
}

// Test multiple write/read cycles
TEST_F(SerialArduinoTest, MultipleEchoCycles)
{
    const char *messages[] = {"Test1\n", "Test2\n", "Test3\n"};
    const int num_messages = 3;

    for (int i = 0; i < num_messages; ++i)
    {
        int msg_len = static_cast<int>(strlen(messages[i]));

        // Write
        int written = serialWrite(handle_, messages[i], msg_len, 1000, 1, nullptr);
        EXPECT_EQ(written, msg_len) << "Cycle " << i << ": write failed";

        // Delay to allow Arduino to process and echo
        usleep(500000); // 500ms - give Arduino more time

        // Read echo
        char read_buffer[256] = {0};
        int read_bytes = serialRead(handle_, read_buffer, sizeof(read_buffer) - 1, 2000, 1, nullptr);
        EXPECT_GT(read_bytes, 0) << "Cycle " << i << ": read failed";
    }
}

// Test timeout behavior
TEST_F(SerialArduinoTest, ReadTimeout)
{
    char buffer[256];
    // Try to read with short timeout when no data is available
    int read_bytes = serialRead(handle_, buffer, sizeof(buffer), 100, 1, nullptr);
    // Should return 0 on timeout (no error, just no data)
    EXPECT_GE(read_bytes, 0) << "Timeout should return 0, not error";
}

// Test invalid handle
TEST(SerialInvalidHandleTest, InvalidHandleRead)
{
    char buffer[256];
    int result = serialRead(-1, buffer, sizeof(buffer), 1000, 1, nullptr);
    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError))
        << "Should return error for invalid handle";
}

TEST(SerialInvalidHandleTest, InvalidHandleWrite)
{
    const char *data = "test";
    int result = serialWrite(-1, data, 4, 1000, 1, nullptr);
    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kInvalidHandleError))
        << "Should return error for invalid handle";
}

TEST(SerialInvalidHandleTest, InvalidHandleClose)
{
    int result = serialClose(-1, nullptr);
    // According to spec, closing invalid handle is a no-op
    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kSuccess));
}
