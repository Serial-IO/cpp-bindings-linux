// Integration tests for a real Arduino-compatible serial device running the echo sketch.

#include <cpp_core/interface/serial_clear_buffer_in.h>
#include <cpp_core/interface/serial_clear_buffer_out.h>
#include <cpp_core/interface/serial_close.h>
#include <cpp_core/interface/serial_drain.h>
#include <cpp_core/interface/serial_get_baudrate.h>
#include <cpp_core/interface/serial_get_data_bits.h>
#include <cpp_core/interface/serial_get_flow_control.h>
#include <cpp_core/interface/serial_get_parity.h>
#include <cpp_core/interface/serial_get_stop_bits.h>
#include <cpp_core/interface/serial_in_bytes_total.h>
#include <cpp_core/interface/serial_in_bytes_waiting.h>
#include <cpp_core/interface/serial_open.h>
#include <cpp_core/interface/serial_out_bytes_total.h>
#include <cpp_core/interface/serial_out_bytes_waiting.h>
#include <cpp_core/interface/serial_read.h>
#include <cpp_core/interface/serial_read_line.h>
#include <cpp_core/interface/serial_read_until.h>
#include <cpp_core/interface/serial_read_until_sequence.h>
#include <cpp_core/interface/serial_set_baudrate.h>
#include <cpp_core/interface/serial_set_data_bits.h>
#include <cpp_core/interface/serial_set_flow_control.h>
#include <cpp_core/interface/serial_set_parity.h>
#include <cpp_core/interface/serial_set_read_callback.h>
#include <cpp_core/interface/serial_set_stop_bits.h>
#include <cpp_core/interface/serial_set_write_callback.h>
#include <cpp_core/interface/serial_write.h>
#include <cpp_core/status_code.h>

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <thread>

namespace
{
constexpr auto kInvalidHandleError = static_cast<int>(cpp_core::StatusCode::Connection::kInvalidHandleError);
constexpr auto kSuccess = static_cast<int>(cpp_core::StatusCode::kSuccess);
constexpr int kDefaultBaudrate = 115200;
constexpr int kOpenResetDelayMs = 2000;
constexpr int kPollIntervalMs = 25;
constexpr int kShortReadTimeoutMs = 150;
constexpr int kEchoTimeoutMs = 3000;

auto sleepForMilliseconds(int milliseconds) -> void
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

struct IoCallbackCounter
{
    static inline IoCallbackCounter *instance = nullptr;

    static void noteRead(int bytes_read)
    {
        if (instance != nullptr)
        {
            instance->read_bytes.fetch_add(bytes_read, std::memory_order_relaxed);
        }
    }

    static void noteWrite(int bytes_written)
    {
        if (instance != nullptr)
        {
            instance->write_bytes.fetch_add(bytes_written, std::memory_order_relaxed);
        }
    }

    std::atomic<int> read_bytes{0};
    std::atomic<int> write_bytes{0};
};
} // namespace

class SerialArduinoTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const char *env_port = std::getenv("SERIAL_TEST_PORT"); // NOLINT(concurrency-mt-unsafe)
        const char *selected_port = (env_port != nullptr && env_port[0] != '\0') ? env_port : "/dev/ttyUSB0";
        handle_ =
            serialOpen(const_cast<void *>(static_cast<const void *>(selected_port)), kDefaultBaudrate, 8, 0, 0, nullptr);

        if (handle_ <= 0)
        {
            GTEST_SKIP() << "Could not open serial port '" << selected_port
                         << "'. Set SERIAL_TEST_PORT or connect Arduino on /dev/ttyUSB0.";
        }

        sleepForMilliseconds(kOpenResetDelayMs);
        ASSERT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);
    }

    void TearDown() override
    {
        serialSetReadCallback(nullptr);
        serialSetWriteCallback(nullptr);
        IoCallbackCounter::instance = nullptr;

        if (handle_ > 0)
        {
            serialClose(handle_, nullptr);
            handle_ = 0;
        }
    }

    auto waitForAvailableBytes(int minimum_bytes, int total_timeout_ms) -> int
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(total_timeout_ms);
        int last_seen = 0;

        while (std::chrono::steady_clock::now() < deadline)
        {
            last_seen = serialInBytesWaiting(handle_, nullptr);
            if (last_seen >= minimum_bytes)
            {
                return last_seen;
            }

            sleepForMilliseconds(kPollIntervalMs);
        }

        return serialInBytesWaiting(handle_, nullptr);
    }

    auto readExact(char *destination, int expected_bytes, int total_timeout_ms) -> int
    {
        if (destination == nullptr || expected_bytes <= 0)
        {
            return 0;
        }

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(total_timeout_ms);
        int total_read = 0;

        while (total_read < expected_bytes && std::chrono::steady_clock::now() < deadline)
        {
            const int chunk =
                serialRead(handle_, destination + total_read, expected_bytes - total_read, kShortReadTimeoutMs, 1, nullptr);
            if (chunk < 0)
            {
                return chunk;
            }
            if (chunk == 0)
            {
                sleepForMilliseconds(kPollIntervalMs);
                continue;
            }

            total_read += chunk;
        }

        return total_read;
    }

    auto roundTripExact(std::string_view message) -> void
    {
        ASSERT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);

        const int message_size = static_cast<int>(message.size());
        ASSERT_GT(message_size, 0);

        const int written = serialWrite(handle_, message.data(), message_size, 1000, 1, nullptr);
        ASSERT_EQ(written, message_size) << "Failed to write full message";
        ASSERT_EQ(serialDrain(handle_, nullptr), kSuccess);

        const int waiting = waitForAvailableBytes(message_size, kEchoTimeoutMs);
        ASSERT_GE(waiting, message_size) << "Timed out waiting for echoed bytes";

        std::array<char, 256> buffer{};
        ASSERT_LE(message_size, static_cast<int>(buffer.size()));
        const int read_bytes = readExact(buffer.data(), message_size, kEchoTimeoutMs);

        ASSERT_EQ(read_bytes, message_size) << "Did not read the complete echo";
        EXPECT_EQ(std::string_view(buffer.data(), static_cast<std::size_t>(read_bytes)), message);
    }

    intptr_t handle_ = 0;
};

TEST_F(SerialArduinoTest, OpenClose)
{
    EXPECT_GT(handle_, 0) << "serialOpen should return a positive handle";
}

TEST_F(SerialArduinoTest, WriteReadEchoMatchesExactly)
{
    roundTripExact("Hello Arduino!\n");
}

TEST_F(SerialArduinoTest, MultipleEchoCyclesMatchExactly)
{
    const std::array<std::string_view, 3> messages = {"Test1\n", "Test2\n", "Test3\n"};

    for (const auto message : messages)
    {
        roundTripExact(message);
    }
}

TEST_F(SerialArduinoTest, ReadTimeoutReturnsZeroWhenNoDataIsPending)
{
    ASSERT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);

    std::array<char, 256> buffer{};
    const int read_bytes = serialRead(handle_, buffer.data(), static_cast<int>(buffer.size()), 100, 1, nullptr);
    EXPECT_EQ(read_bytes, 0);
}

TEST_F(SerialArduinoTest, ReadLineStopsAtNewline)
{
    ASSERT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);

    constexpr std::string_view message = "Line helper test\n";
    ASSERT_EQ(serialWrite(handle_, message.data(), static_cast<int>(message.size()), 1000, 1, nullptr),
              static_cast<int>(message.size()));

    std::array<char, 256> buffer{};
    const int read_bytes =
        serialReadLine(handle_, buffer.data(), static_cast<int>(buffer.size()), kEchoTimeoutMs, 1, nullptr);

    ASSERT_EQ(read_bytes, static_cast<int>(message.size()));
    EXPECT_EQ(std::string_view(buffer.data(), static_cast<std::size_t>(read_bytes)), message);
}

TEST_F(SerialArduinoTest, ReadUntilStopsAtRequestedByte)
{
    ASSERT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);

    constexpr std::string_view message = "Echo until!";
    constexpr char terminator = '!';

    ASSERT_EQ(serialWrite(handle_, message.data(), static_cast<int>(message.size()), 1000, 1, nullptr),
              static_cast<int>(message.size()));

    std::array<char, 256> buffer{};
    const int read_bytes = serialReadUntil(handle_, buffer.data(), static_cast<int>(buffer.size()), kEchoTimeoutMs, 1,
                                           const_cast<char *>(&terminator), nullptr);

    ASSERT_EQ(read_bytes, static_cast<int>(message.size()));
    EXPECT_EQ(std::string_view(buffer.data(), static_cast<std::size_t>(read_bytes)), message);
}

TEST_F(SerialArduinoTest, ReadUntilSequenceStopsAtRequestedSuffix)
{
    ASSERT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);

    constexpr std::string_view message = "prefix-END";
    char sequence[] = "END";

    ASSERT_EQ(serialWrite(handle_, message.data(), static_cast<int>(message.size()), 1000, 1, nullptr),
              static_cast<int>(message.size()));

    std::array<char, 256> buffer{};
    const int read_bytes = serialReadUntilSequence(handle_, buffer.data(), static_cast<int>(buffer.size()),
                                                   kEchoTimeoutMs, 1, sequence, nullptr);

    ASSERT_EQ(read_bytes, static_cast<int>(message.size()));
    EXPECT_EQ(std::string_view(buffer.data(), static_cast<std::size_t>(read_bytes)), message);
}

TEST_F(SerialArduinoTest, ByteCountersAndCallbacksTrackRealTraffic)
{
    IoCallbackCounter callback_counter;
    IoCallbackCounter::instance = &callback_counter;
    serialSetReadCallback(&IoCallbackCounter::noteRead);
    serialSetWriteCallback(&IoCallbackCounter::noteWrite);

    constexpr std::string_view message = "Callback bytes\n";
    roundTripExact(message);

    EXPECT_EQ(serialOutBytesTotal(handle_, nullptr), static_cast<int64_t>(message.size()));
    EXPECT_EQ(serialInBytesTotal(handle_, nullptr), static_cast<int64_t>(message.size()));
    EXPECT_EQ(callback_counter.write_bytes.load(std::memory_order_relaxed), static_cast<int>(message.size()));
    EXPECT_EQ(callback_counter.read_bytes.load(std::memory_order_relaxed), static_cast<int>(message.size()));
}

TEST_F(SerialArduinoTest, CanObserveAndClearPendingInput)
{
    ASSERT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);

    constexpr std::string_view message = "Buffered input\n";
    ASSERT_EQ(serialWrite(handle_, message.data(), static_cast<int>(message.size()), 1000, 1, nullptr),
              static_cast<int>(message.size()));

    const int waiting = waitForAvailableBytes(static_cast<int>(message.size()), kEchoTimeoutMs);
    ASSERT_GE(waiting, static_cast<int>(message.size()));

    EXPECT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);
    sleepForMilliseconds(100);
    EXPECT_EQ(serialInBytesWaiting(handle_, nullptr), 0);

    std::array<char, 64> buffer{};
    EXPECT_EQ(serialRead(handle_, buffer.data(), static_cast<int>(buffer.size()), 100, 1, nullptr), 0);
}

TEST_F(SerialArduinoTest, CanRoundTripLineSettingsAndRecoverCommunication)
{
    EXPECT_EQ(serialGetBaudrate(handle_, nullptr), kDefaultBaudrate);
    EXPECT_EQ(serialGetDataBits(handle_, nullptr), 8);
    EXPECT_EQ(serialGetParity(handle_, nullptr), 0);
    EXPECT_EQ(serialGetStopBits(handle_, nullptr), 0);
    EXPECT_EQ(serialGetFlowControl(handle_, nullptr), 0);

    ASSERT_EQ(serialSetBaudrate(handle_, 57600, nullptr), kSuccess);
    EXPECT_EQ(serialGetBaudrate(handle_, nullptr), 57600);
    ASSERT_EQ(serialSetBaudrate(handle_, kDefaultBaudrate, nullptr), kSuccess);
    EXPECT_EQ(serialGetBaudrate(handle_, nullptr), kDefaultBaudrate);

    ASSERT_EQ(serialSetDataBits(handle_, 7, nullptr), kSuccess);
    EXPECT_EQ(serialGetDataBits(handle_, nullptr), 7);
    ASSERT_EQ(serialSetDataBits(handle_, 8, nullptr), kSuccess);
    EXPECT_EQ(serialGetDataBits(handle_, nullptr), 8);

    ASSERT_EQ(serialSetParity(handle_, 2, nullptr), kSuccess);
    EXPECT_EQ(serialGetParity(handle_, nullptr), 2);
    ASSERT_EQ(serialSetParity(handle_, 0, nullptr), kSuccess);
    EXPECT_EQ(serialGetParity(handle_, nullptr), 0);

    ASSERT_EQ(serialSetStopBits(handle_, 2, nullptr), kSuccess);
    EXPECT_EQ(serialGetStopBits(handle_, nullptr), 2);
    ASSERT_EQ(serialSetStopBits(handle_, 0, nullptr), kSuccess);
    EXPECT_EQ(serialGetStopBits(handle_, nullptr), 0);

    ASSERT_EQ(serialSetFlowControl(handle_, 2, nullptr), kSuccess);
    EXPECT_EQ(serialGetFlowControl(handle_, nullptr), 2);
    ASSERT_EQ(serialSetFlowControl(handle_, 0, nullptr), kSuccess);
    EXPECT_EQ(serialGetFlowControl(handle_, nullptr), 0);

    // USB CDC devices can need a short resync window after multiple line-coding changes.
    sleepForMilliseconds(150);
    ASSERT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);
    roundTripExact("Configuration restored\n");
}

TEST_F(SerialArduinoTest, IdleOutputControlFunctionsSucceed)
{
    EXPECT_EQ(serialOutBytesWaiting(handle_, nullptr), 0);
    EXPECT_EQ(serialDrain(handle_, nullptr), kSuccess);
    EXPECT_EQ(serialClearBufferOut(handle_, nullptr), kSuccess);
}

TEST(SerialInvalidHandleTest, InvalidHandleRead)
{
    std::array<char, 256> buffer{};
    const int result = serialRead(-1, buffer.data(), static_cast<int>(buffer.size()), 1000, 1, nullptr);
    EXPECT_EQ(result, kInvalidHandleError) << "Should return error for invalid handle";
}

TEST(SerialInvalidHandleTest, InvalidHandleWrite)
{
    const char *data = "test";
    const int result = serialWrite(-1, data, 4, 1000, 1, nullptr);
    EXPECT_EQ(result, kInvalidHandleError) << "Should return error for invalid handle";
}

TEST(SerialInvalidHandleTest, InvalidHandleClose)
{
    const int result = serialClose(-1, nullptr);
    EXPECT_EQ(result, kSuccess);
}
