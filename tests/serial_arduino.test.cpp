// Integration test: serial communication with Arduino echo script on /dev/ttyUSB0

#include <cpp_core/interface/serial_abort_read.h>
#include <cpp_core/interface/serial_close.h>
#include <cpp_core/interface/serial_open.h>
#include <cpp_core/interface/serial_read.h>
#include <cpp_core/interface/serial_write.h>
#include <cpp_core/serial.h>
#include <cpp_core/status_codes.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <array>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <unistd.h>

class SerialArduinoTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const char *env_port = std::getenv("SERIAL_TEST_PORT"); // NOLINT(concurrency-mt-unsafe)
        const char *port = env_port != nullptr ? env_port : "/dev/ttyUSB0";
        handle = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 115200, 8, 0, 0, nullptr);

        if (handle <= 0)
        {
            GTEST_SKIP() << "Could not open serial port '" << port
                         << "'. Set SERIAL_TEST_PORT or connect Arduino on /dev/ttyUSB0.";
        }

        const char *skip_delay = std::getenv("SERIAL_TEST_SKIP_INIT_DELAY");
        if (skip_delay == nullptr || std::strcmp(skip_delay, "1") != 0)
        {
            usleep(2000000);
        }
    }

    void TearDown() override
    {
        if (handle > 0)
        {
            serialClose(handle, nullptr);
            handle = 0;
        }
    }

    intptr_t handle = 0;
};

TEST_F(SerialArduinoTest, OpenClose)
{
    EXPECT_GT(handle, 0) << "serialOpen should return a positive handle";
}

TEST_F(SerialArduinoTest, WriteReadEcho)
{
    const char *test_message = "Hello Arduino!\n";
    int message_len = static_cast<int>(strlen(test_message));

    int written = serialWrite(handle, test_message, message_len, 1000, 1, nullptr);
    EXPECT_EQ(written, message_len) << "Should write all bytes. Written: " << written << ", Expected: " << message_len;

    usleep(500000);

    std::array<char, 256> read_buffer{};
    int read_bytes = serialRead(handle, read_buffer.data(), static_cast<int>(read_buffer.size()) - 1, 2000, 1, nullptr);

    EXPECT_GT(read_bytes, 0) << "Should read at least some bytes";
    EXPECT_LE(read_bytes, static_cast<int>(read_buffer.size()) - 1) << "Should not overflow buffer";

    read_buffer[static_cast<size_t>(read_bytes)] = '\0';
    EXPECT_STRNE(read_buffer.data(), "") << "Should receive echo from Arduino";
}

TEST_F(SerialArduinoTest, MultipleEchoCycles)
{
    const std::array<const char *, 3> messages = {"Test1\n", "Test2\n", "Test3\n"};

    for (size_t i = 0; i < messages.size(); ++i)
    {
        int msg_len = static_cast<int>(strlen(messages[i]));

        int written = serialWrite(handle, messages[i], msg_len, 1000, 1, nullptr);
        EXPECT_EQ(written, msg_len) << "Cycle " << i << ": write failed";

        usleep(500000);

        std::array<char, 256> read_buffer{};
        int read_bytes =
            serialRead(handle, read_buffer.data(), static_cast<int>(read_buffer.size()) - 1, 2000, 1, nullptr);
        EXPECT_GT(read_bytes, 0) << "Cycle " << i << ": read failed";
    }
}

TEST_F(SerialArduinoTest, ReadTimeout)
{
    std::array<char, 256> buffer{};
    int read_bytes = serialRead(handle, buffer.data(), static_cast<int>(buffer.size()), 100, 1, nullptr);
    EXPECT_GE(read_bytes, 0) << "Timeout should return 0, not error";
}

TEST_F(SerialArduinoTest, AbortRead)
{
    std::atomic<int> read_result{999};
    std::thread reader([&] {
        unsigned char buffer[16] = {};
        read_result.store(serialRead(handle_, buffer, sizeof(buffer), 10000, 1, nullptr));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(serialAbortRead(handle_, nullptr), 0);

    reader.join();
    EXPECT_EQ(read_result.load(), static_cast<int>(cpp_core::StatusCodes::kAbortReadError));
}

TEST(SerialInvalidHandleTest, InvalidHandleRead)
{
    std::array<char, 256> buffer{};
    int result = serialRead(-1, buffer.data(), static_cast<int>(buffer.size()), 1000, 1, nullptr);
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
    EXPECT_EQ(result, static_cast<int>(cpp_core::StatusCodes::kSuccess));
}
