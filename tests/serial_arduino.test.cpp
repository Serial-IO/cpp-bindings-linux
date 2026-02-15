// Integration test: serial communication with Arduino echo script on /dev/ttyUSB0

#include <cpp_core/interface/serial_abort_read.h>
#include <cpp_core/interface/serial_abort_write.h>
#include <cpp_core/interface/serial_close.h>
#include <cpp_core/interface/serial_open.h>
#include <cpp_core/interface/serial_read.h>
#include <cpp_core/interface/serial_write.h>
#include <cpp_core/status_codes.h>

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <array>
#include <cstring>
#include <string>
#include <thread>
#include <unistd.h>
#include <unistd.h>

namespace
{
auto drainInput(intptr_t handle) -> void
{
    std::array<unsigned char, 256> tmp = {};
    for (;;)
    {
        const int res = serialRead(handle, tmp.data(), static_cast<int>(tmp.size()), 10, 0, nullptr);
        if (res < 0)
        {
            FAIL() << "drainInput failed with error " << res;
        }
        if (res == 0)
        {
            return;
        }
    }
}
} // namespace

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
            GTEST_FAIL() << "Could not open serial port '" << port
                         << "'. Set SERIAL_TEST_PORT or connect Arduino on /dev/ttyUSB0.";
        }

        // Heuristic: real Arduinos often reset on open; CI pseudo TTYs do not.
        const bool looks_like_ci_pty = (std::strstr(port, "/tmp/ttyCI_") != nullptr);
        if (!looks_like_ci_pty)
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

TEST_F(SerialArduinoTest, Read60BytesWithoutWritingTimesOut)
{
    drainInput(handle);
    std::array<unsigned char, 60> buffer = {};
    const int read_bytes = serialRead(handle, buffer.data(), static_cast<int>(buffer.size()), 200, 1, nullptr);
    EXPECT_EQ(read_bytes, 0) << "Expected timeout (0 bytes) when no data is sent";
}

TEST_F(SerialArduinoTest, Write10BytesRead60Returns10ThenTimesOut)
{
    drainInput(handle);
    const std::array<unsigned char, 10> payload = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    const int written = serialWrite(handle, payload.data(), static_cast<int>(payload.size()), 2000, 1, nullptr);
    ASSERT_EQ(written, static_cast<int>(payload.size()));

    std::array<unsigned char, 60> buffer = {};
    const int read_bytes = serialRead(handle, buffer.data(), static_cast<int>(buffer.size()), 200, 1, nullptr);
    EXPECT_EQ(read_bytes, static_cast<int>(payload.size()))
        << "Expected to read the 10 echoed bytes, then timeout waiting for more";
}

TEST_F(SerialArduinoTest, AbortRead)
{
    std::atomic<int> read_result{999};
    std::thread reader([&] {
        std::array<unsigned char, 16> buffer = {};
        read_result.store(serialRead(handle, buffer.data(), static_cast<int>(buffer.size()), 10000, 1, nullptr));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(serialAbortRead(handle, nullptr), 0);

    reader.join();
    EXPECT_EQ(read_result.load(), static_cast<int>(cpp_core::StatusCodes::kAbortReadError));
}

TEST_F(SerialArduinoTest, AbortWriteDuringLargeTransfer)
{
    const int64_t default_total_bytes = 100LL * 1024LL * 1024LL;
    const int64_t default_abort_after_bytes = 1LL * 1024LL * 1024LL;
    const int64_t total_bytes = default_total_bytes;
    const int64_t abort_after_bytes = default_abort_after_bytes;

    // We intentionally do NOT read the echo here; the goal is to saturate the OS TX queue so
    // serialWrite() hits EAGAIN -> poll(), then verify serialAbortWrite() unblocks it.
    constexpr int kChunkSize = 64 * 1024;
    static_assert(kChunkSize > 0);

    std::string chunk(static_cast<size_t>(kChunkSize), '\x55');
    std::atomic<int64_t> bytes_sent{0};
    std::atomic<bool> in_write_call{false};
    std::atomic<int> write_result{999};

    std::thread writer([&] {
        while (bytes_sent.load() < total_bytes)
        {
            in_write_call.store(true);
            const int res = serialWrite(handle, chunk.data(), static_cast<int>(chunk.size()), 10000, 1, nullptr);
            in_write_call.store(false);

            if (res == static_cast<int>(cpp_core::StatusCodes::kAbortWriteError))
            {
                write_result.store(res);
                return;
            }
            if (res < 0)
            {
                write_result.store(res);
                return;
            }
            bytes_sent.fetch_add(res);
        }
        write_result.store(static_cast<int>(cpp_core::StatusCodes::kSuccess));
    });

    // Wait briefly for either progress OR an early error, then start aborting.
    const auto wait_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (std::chrono::steady_clock::now() < wait_deadline)
    {
        if (write_result.load() != 999)
        {
            break;
        }
        if (bytes_sent.load() >= abort_after_bytes)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    const auto abort_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < abort_deadline)
    {
        if (write_result.load() != 999)
        {
            break;
        }
        EXPECT_EQ(serialAbortWrite(handle, nullptr), 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    // Hard fail-safe: if abort did not take effect, close the handle to force the writer to exit.
    if (write_result.load() != static_cast<int>(cpp_core::StatusCodes::kAbortWriteError))
    {
        (void)serialClose(handle, nullptr);
        handle = 0;
    }

    writer.join();

    EXPECT_EQ(write_result.load(), static_cast<int>(cpp_core::StatusCodes::kAbortWriteError))
        << "Expected abort during large transfer. bytes_sent=" << bytes_sent.load() << " total_bytes=" << total_bytes;
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
