// Integration tests for a real Arduino-compatible serial device running the echo sketch.

#include <cpp_core/interface/serial_clear_buffer_in.h>
#include <cpp_core/interface/serial_clear_buffer_out.h>
#include <cpp_core/interface/serial_close.h>
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
#include <cpp_core/interface/serial_read_until_sequence.h>
#include <cpp_core/interface/serial_set_baudrate.h>
#include <cpp_core/interface/serial_set_data_bits.h>
#include <cpp_core/interface/serial_set_flow_control.h>
#include <cpp_core/interface/serial_set_parity.h>
#include <cpp_core/interface/serial_set_read_callback.h>
#include <cpp_core/interface/serial_set_stop_bits.h>
#include <cpp_core/interface/serial_set_write_callback.h>
#include <cpp_core/interface/serial_wait_for_drain.h>
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

auto runningInGitHubActions() -> bool
{
    const char *value = std::getenv("GITHUB_ACTIONS"); // NOLINT(concurrency-mt-unsafe)
    return value != nullptr && std::strcmp(value, "true") == 0;
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
        const cpp_core::SerialConfig config0{kDefaultBaudrate, cpp_core::DataBits::kEight, cpp_core::Parity::kNone,
                                             cpp_core::StopBits::kOne, cpp_core::FlowControl::kNone};
        handle_ = serialOpen(selected_port, &config0, nullptr);

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
            const cpp_core::SerialTimeoutConfig timeout_config1{kShortReadTimeoutMs, 1};
            const int chunk = serialRead(handle_, reinterpret_cast<std::uint8_t *>(destination + total_read),
                                         expected_bytes - total_read, &timeout_config1, nullptr);
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

        const cpp_core::SerialTimeoutConfig timeout_config2{1000, 1};
        const int written = serialWrite(handle_, reinterpret_cast<const std::uint8_t *>(message.data()), message_size,
                                        &timeout_config2, nullptr);
        ASSERT_EQ(written, message_size) << "Failed to write full message";
        ASSERT_EQ(serialWaitForDrain(handle_, nullptr), kSuccess);

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
    const cpp_core::SerialTimeoutConfig timeout_config3{100, 1};
    const int read_bytes = serialRead(handle_, reinterpret_cast<std::uint8_t *>(buffer.data()),
                                      static_cast<int>(buffer.size()), &timeout_config3, nullptr);
    EXPECT_EQ(read_bytes, 0);
}

TEST_F(SerialArduinoTest, ReadLineStopsAtNewline)
{
    ASSERT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);

    constexpr std::string_view message = "Line helper test\n";
    const cpp_core::SerialTimeoutConfig timeout_config4{1000, 1};
    ASSERT_EQ(serialWrite(handle_, reinterpret_cast<const std::uint8_t *>(message.data()),
                          static_cast<int>(message.size()), &timeout_config4, nullptr),
              static_cast<int>(message.size()));

    std::array<char, 256> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config5{kEchoTimeoutMs, 1};
    const int read_bytes = serialReadUntilSequence(handle_, reinterpret_cast<std::uint8_t *>(buffer.data()),
                                                   static_cast<int>(buffer.size()), &timeout_config5,
                                                   reinterpret_cast<const std::uint8_t *>("\n"), 1, nullptr);

    ASSERT_EQ(read_bytes, static_cast<int>(message.size()));
    EXPECT_EQ(std::string_view(buffer.data(), static_cast<std::size_t>(read_bytes)), message);
}

TEST_F(SerialArduinoTest, ReadUntilStopsAtRequestedByte)
{
    ASSERT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);

    constexpr std::string_view message = "Echo until!";
    constexpr char terminator = '!';

    const cpp_core::SerialTimeoutConfig timeout_config6{1000, 1};
    ASSERT_EQ(serialWrite(handle_, reinterpret_cast<const std::uint8_t *>(message.data()),
                          static_cast<int>(message.size()), &timeout_config6, nullptr),
              static_cast<int>(message.size()));

    std::array<char, 256> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config7{kEchoTimeoutMs, 1};
    const int read_bytes = serialReadUntilSequence(
        handle_, reinterpret_cast<std::uint8_t *>(buffer.data()), static_cast<int>(buffer.size()), &timeout_config7,
        reinterpret_cast<const std::uint8_t *>(const_cast<char *>(&terminator)), 1, nullptr);

    ASSERT_EQ(read_bytes, static_cast<int>(message.size()));
    EXPECT_EQ(std::string_view(buffer.data(), static_cast<std::size_t>(read_bytes)), message);
}

TEST_F(SerialArduinoTest, ReadUntilSequenceStopsAtRequestedSuffix)
{
    ASSERT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);

    constexpr std::string_view message = "prefix-END";
    char sequence[] = "END";

    const cpp_core::SerialTimeoutConfig timeout_config8{1000, 1};
    ASSERT_EQ(serialWrite(handle_, reinterpret_cast<const std::uint8_t *>(message.data()),
                          static_cast<int>(message.size()), &timeout_config8, nullptr),
              static_cast<int>(message.size()));

    std::array<char, 256> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config9{kEchoTimeoutMs, 1};
    const int read_bytes = serialReadUntilSequence(handle_, reinterpret_cast<std::uint8_t *>(buffer.data()),
                                                   static_cast<int>(buffer.size()), &timeout_config9,
                                                   reinterpret_cast<const std::uint8_t *>(sequence), 3, nullptr);

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
    const cpp_core::SerialTimeoutConfig timeout_config10{1000, 1};
    ASSERT_EQ(serialWrite(handle_, reinterpret_cast<const std::uint8_t *>(message.data()),
                          static_cast<int>(message.size()), &timeout_config10, nullptr),
              static_cast<int>(message.size()));

    const int waiting = waitForAvailableBytes(static_cast<int>(message.size()), kEchoTimeoutMs);
    ASSERT_GE(waiting, static_cast<int>(message.size()));

    EXPECT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);
    sleepForMilliseconds(100);
    EXPECT_EQ(serialInBytesWaiting(handle_, nullptr), 0);

    std::array<char, 64> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config11{100, 1};
    EXPECT_EQ(serialRead(handle_, reinterpret_cast<std::uint8_t *>(buffer.data()), static_cast<int>(buffer.size()),
                         &timeout_config11, nullptr),
              0);
}

TEST_F(SerialArduinoTest, CanRoundTripLineSettingsAndRecoverCommunication)
{
    if (runningInGitHubActions())
    {
        GTEST_SKIP() << "Virtual CI serial endpoints do not reliably support the line-setting roundtrip semantics.";
    }

    EXPECT_EQ(serialGetBaudrate(handle_, nullptr), kDefaultBaudrate);
    EXPECT_EQ(serialGetDataBits(handle_, nullptr), cpp_core::DataBits::kEight);
    EXPECT_EQ(serialGetParity(handle_, nullptr), cpp_core::Parity::kNone);
    EXPECT_EQ(serialGetStopBits(handle_, nullptr), cpp_core::StopBits::kOne);
    EXPECT_EQ(serialGetFlowControl(handle_, nullptr), cpp_core::FlowControl::kNone);

    ASSERT_EQ(serialSetBaudrate(handle_, 57600, nullptr), kSuccess);
    EXPECT_EQ(serialGetBaudrate(handle_, nullptr), 57600);
    ASSERT_EQ(serialSetBaudrate(handle_, kDefaultBaudrate, nullptr), kSuccess);
    EXPECT_EQ(serialGetBaudrate(handle_, nullptr), kDefaultBaudrate);

    ASSERT_EQ(serialSetDataBits(handle_, cpp_core::DataBits::kSeven, nullptr), kSuccess);
    EXPECT_EQ(serialGetDataBits(handle_, nullptr), cpp_core::DataBits::kSeven);
    ASSERT_EQ(serialSetDataBits(handle_, cpp_core::DataBits::kEight, nullptr), kSuccess);
    EXPECT_EQ(serialGetDataBits(handle_, nullptr), cpp_core::DataBits::kEight);

    ASSERT_EQ(serialSetParity(handle_, cpp_core::Parity::kOdd, nullptr), kSuccess);
    EXPECT_EQ(serialGetParity(handle_, nullptr), cpp_core::Parity::kOdd);
    ASSERT_EQ(serialSetParity(handle_, cpp_core::Parity::kNone, nullptr), kSuccess);
    EXPECT_EQ(serialGetParity(handle_, nullptr), cpp_core::Parity::kNone);

    ASSERT_EQ(serialSetStopBits(handle_, cpp_core::StopBits::kTwo, nullptr), kSuccess);
    EXPECT_EQ(serialGetStopBits(handle_, nullptr), cpp_core::StopBits::kTwo);
    ASSERT_EQ(serialSetStopBits(handle_, cpp_core::StopBits::kOne, nullptr), kSuccess);
    EXPECT_EQ(serialGetStopBits(handle_, nullptr), cpp_core::StopBits::kOne);

    ASSERT_EQ(serialSetFlowControl(handle_, cpp_core::FlowControl::kXonXoff, nullptr), kSuccess);
    EXPECT_EQ(serialGetFlowControl(handle_, nullptr), cpp_core::FlowControl::kXonXoff);
    ASSERT_EQ(serialSetFlowControl(handle_, cpp_core::FlowControl::kNone, nullptr), kSuccess);
    EXPECT_EQ(serialGetFlowControl(handle_, nullptr), cpp_core::FlowControl::kNone);

    // USB CDC devices can need a short resync window after multiple line-coding changes.
    sleepForMilliseconds(150);
    ASSERT_EQ(serialClearBufferIn(handle_, nullptr), kSuccess);
    roundTripExact("Configuration restored\n");
}

TEST_F(SerialArduinoTest, IdleOutputControlFunctionsSucceed)
{
    EXPECT_EQ(serialOutBytesWaiting(handle_, nullptr), 0);
    EXPECT_EQ(serialWaitForDrain(handle_, nullptr), kSuccess);
    EXPECT_EQ(serialClearBufferOut(handle_, nullptr), kSuccess);
}

TEST(SerialInvalidHandleTest, InvalidHandleRead)
{
    std::array<char, 256> buffer{};
    const cpp_core::SerialTimeoutConfig timeout_config12{1000, 1};
    const int result = serialRead(-1, reinterpret_cast<std::uint8_t *>(buffer.data()), static_cast<int>(buffer.size()),
                                  &timeout_config12, nullptr);
    EXPECT_EQ(result, kInvalidHandleError) << "Should return error for invalid handle";
}

TEST(SerialInvalidHandleTest, InvalidHandleWrite)
{
    const char *data = "test";
    const cpp_core::SerialTimeoutConfig timeout_config13{1000, 1};
    const int result = serialWrite(-1, reinterpret_cast<const std::uint8_t *>(data), 4, &timeout_config13, nullptr);
    EXPECT_EQ(result, kInvalidHandleError) << "Should return error for invalid handle";
}

TEST(SerialInvalidHandleTest, InvalidHandleClose)
{
    const int result = serialClose(-1, nullptr);
    EXPECT_EQ(result, kSuccess);
}
