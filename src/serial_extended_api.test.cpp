#include <cpp_core/interface/serial_abort_read.h>
#include <cpp_core/interface/serial_in_bytes_total.h>
#include <cpp_core/interface/serial_in_bytes_waiting.h>
#include <cpp_core/interface/serial_list_ports.h>
#include <cpp_core/interface/serial_out_bytes_total.h>
#include <cpp_core/interface/serial_read.h>
#include <cpp_core/interface/serial_read_line.h>
#include <cpp_core/interface/serial_read_until.h>
#include <cpp_core/interface/serial_read_until_sequence.h>
#include <cpp_core/interface/serial_set_error_callback.h>
#include <cpp_core/interface/serial_set_read_callback.h>
#include <cpp_core/interface/serial_set_write_callback.h>
#include <cpp_core/interface/serial_write.h>
#include <cpp_core/status_code.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <thread>
#include <unistd.h>

#include <gtest/gtest.h>

namespace
{
std::atomic<int> g_last_error_code{0};
std::atomic<int> g_last_read_callback{0};
std::atomic<int> g_last_write_callback{0};
std::atomic<int> g_port_callback_count{0};

void globalErrorCallback(int code, const char * /*message*/)
{
    g_last_error_code.store(code, std::memory_order_relaxed);
}

void globalReadCallback(int bytes_read)
{
    g_last_read_callback.store(bytes_read, std::memory_order_relaxed);
}

void globalWriteCallback(int bytes_written)
{
    g_last_write_callback.store(bytes_written, std::memory_order_relaxed);
}

void listPortsCallback(const char * /*port*/, const char * /*path*/, const char * /*manufacturer*/,
                       const char * /*serial_number*/, const char * /*pnp_id*/, const char * /*location_id*/,
                       const char * /*product_id*/, const char * /*vendor_id*/)
{
    g_port_callback_count.fetch_add(1, std::memory_order_relaxed);
}

constexpr auto kAbortReadError = static_cast<int>(cpp_core::StatusCode::Io::kAbortReadError);
constexpr auto kInvalidHandleError = static_cast<int>(cpp_core::StatusCode::Connection::kInvalidHandleError);
} // namespace

class SerialExtendedApiTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        g_last_error_code.store(0, std::memory_order_relaxed);
        g_last_read_callback.store(0, std::memory_order_relaxed);
        g_last_write_callback.store(0, std::memory_order_relaxed);
        g_port_callback_count.store(0, std::memory_order_relaxed);
        serialSetErrorCallback(nullptr);
        serialSetReadCallback(nullptr);
        serialSetWriteCallback(nullptr);
    }
};

TEST_F(SerialExtendedApiTest, GlobalErrorCallbackActsAsFallback)
{
    serialSetErrorCallback(globalErrorCallback);

    std::array<char, 4> buffer{};
    EXPECT_EQ(serialRead(-1, buffer.data(), static_cast<int>(buffer.size()), 10, 1, nullptr), kInvalidHandleError);
    EXPECT_EQ(g_last_error_code.load(std::memory_order_relaxed), kInvalidHandleError);
}

TEST_F(SerialExtendedApiTest, ReadWriteCallbacksAndTotalsTrackPipeHandles)
{
    std::array<int, 2> pipefd{};
    ASSERT_EQ(pipe(pipefd.data()), 0);
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

    serialSetReadCallback(globalReadCallback);
    serialSetWriteCallback(globalWriteCallback);

    const char *message = "hello";
    ASSERT_EQ(serialWrite(pipefd[1], message, 5, 100, 1, nullptr), 5);

    std::array<char, 16> buffer{};
    ASSERT_EQ(serialInBytesWaiting(pipefd[0], nullptr), 5);
    ASSERT_EQ(serialRead(pipefd[0], buffer.data(), 5, 100, 1, nullptr), 5);

    EXPECT_EQ(std::string(buffer.data(), 5), "hello");
    EXPECT_EQ(g_last_write_callback.load(std::memory_order_relaxed), 5);
    EXPECT_EQ(g_last_read_callback.load(std::memory_order_relaxed), 5);
    EXPECT_EQ(serialOutBytesTotal(pipefd[1], nullptr), 5);
    EXPECT_EQ(serialInBytesTotal(pipefd[0], nullptr), 5);

    close(pipefd[0]);
    close(pipefd[1]);
}

TEST_F(SerialExtendedApiTest, ReadHelpersStopAtRequestedTerminator)
{
    {
        std::array<int, 2> pipefd{};
        ASSERT_EQ(pipe(pipefd.data()), 0);
        fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
        fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

        const char *line = "alpha\nbeta";
        ASSERT_EQ(write(pipefd[1], line, std::strlen(line)), static_cast<ssize_t>(std::strlen(line)));

        std::array<char, 16> line_buffer{};
        ASSERT_EQ(serialReadLine(pipefd[0], line_buffer.data(), static_cast<int>(line_buffer.size()), 100, 1, nullptr),
                  6);
        EXPECT_EQ(std::string(line_buffer.data(), 6), "alpha\n");

        close(pipefd[0]);
        close(pipefd[1]);
    }

    {
        std::array<int, 2> pipefd{};
        ASSERT_EQ(pipe(pipefd.data()), 0);
        fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
        fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

        const char *payload = "prefix-END-tail";
        ASSERT_EQ(write(pipefd[1], payload, std::strlen(payload)), static_cast<ssize_t>(std::strlen(payload)));

        std::array<char, 32> until_buffer{};
        unsigned char dash = '-';
        ASSERT_EQ(serialReadUntil(pipefd[0], until_buffer.data(), static_cast<int>(until_buffer.size()), 100, 1, &dash,
                                  nullptr),
                  7);
        EXPECT_EQ(std::string(until_buffer.data(), 7), "prefix-");

        close(pipefd[0]);
        close(pipefd[1]);
    }

    {
        std::array<int, 2> pipefd{};
        ASSERT_EQ(pipe(pipefd.data()), 0);
        fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
        fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

        const char *payload = "more-END-rest";
        ASSERT_EQ(write(pipefd[1], payload, std::strlen(payload)), static_cast<ssize_t>(std::strlen(payload)));

        std::array<char, 32> sequence_buffer{};
        ASSERT_EQ(serialReadUntilSequence(pipefd[0], sequence_buffer.data(), static_cast<int>(sequence_buffer.size()),
                                          100, 1, const_cast<char *>("END"), nullptr),
                  8);
        EXPECT_EQ(std::string(sequence_buffer.data(), 8), "more-END");

        close(pipefd[0]);
        close(pipefd[1]);
    }
}

TEST_F(SerialExtendedApiTest, AbortReadInterruptsWaitingOperation)
{
    std::array<int, 2> pipefd{};
    ASSERT_EQ(pipe(pipefd.data()), 0);
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

    std::array<char, 8> buffer{};
    int read_result = 0;
    std::thread reader([&] {
        read_result = serialRead(pipefd[0], buffer.data(), static_cast<int>(buffer.size()), 2000, 1, nullptr);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(serialAbortRead(pipefd[0], nullptr), 0);
    reader.join();

    EXPECT_EQ(read_result, kAbortReadError);

    close(pipefd[0]);
    close(pipefd[1]);
}

TEST_F(SerialExtendedApiTest, ListPortsDoesNotFailOnValidCallback)
{
    const int result = serialListPorts(listPortsCallback, nullptr);
    EXPECT_GE(result, 0);
    EXPECT_EQ(result, g_port_callback_count.load(std::memory_order_relaxed));
}
