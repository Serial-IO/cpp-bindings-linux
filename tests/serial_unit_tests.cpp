#include "serial.h"
#include "status_codes.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <unistd.h>

namespace
{
// Helper storage for callback tests
std::atomic<int>* g_err_ptr = nullptr;

void errorCallback(int code, const char* /*message*/)
{
    if (g_err_ptr != nullptr)
    {
        *g_err_ptr = code;
    }
}

// Helper to resolve serial port path (env var override)
const char* getDefaultPort()
{
    const char* env = std::getenv("SERIAL_PORT");
    return (env != nullptr) ? env : "/dev/ttyUSB0";
}

struct SerialDevice
{
    intptr_t handle{0};
    const char* port{nullptr};

    explicit SerialDevice(int baud = 115200)
    {
        port = getDefaultPort();
        handle = serialOpen((void*)port, baud, 8, 0, 0);
        if (handle == 0)
        {
            throw std::runtime_error(std::string{"Failed to open port "} + port);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // give Arduino time to reboot after DTR toggle
    }

    ~SerialDevice()
    {
        if (handle != 0)
        {
            serialClose(handle);
        }
    }

    void writeToDevice(std::string_view data) const
    {
        serialWrite(handle, data.data(), static_cast<int>(data.size()), 500, 1);
    }
};

} // namespace

// ------------------------------- Error path --------------------------------
TEST(SerialOpenTest, InvalidPathInvokesErrorCallback)
{
    std::atomic<int> err_code{0};

    g_err_ptr = &err_code;
    serialOnError(errorCallback);

    intptr_t handle = serialOpen((void*)"/dev/__does_not_exist__", 115200, 8, 0, 0);
    EXPECT_EQ(handle, 0);
    EXPECT_EQ(err_code.load(), static_cast<int>(StatusCodes::INVALID_HANDLE_ERROR));

    // Reset to nullptr so other tests don't see our callback
    serialOnError(nullptr);
}

// ------------------------ serialGetPortsInfo checks ------------------------
TEST(SerialGetPortsInfoTest, CallbackReceivesPortInfo)
{
    // Counter for how many times the callback is invoked
    static int callback_count = 0;
    callback_count = 0;

    auto callbackFunc = [](const char* /*port*/,
                           const char* /*path*/,
                           const char* /*manufacturer*/,
                           const char* /*serialNumber*/,
                           const char* /*pnpId*/,
                           const char* /*locationId*/,
                           const char* /*productId*/,
                           const char* /*vendorId*/) { ++callback_count; };

    std::atomic<int> err_code{0};
    g_err_ptr = &err_code;
    serialOnError(errorCallback);

    int result = serialGetPortsInfo(callbackFunc);

    // result should match the number of times our callback ran
    EXPECT_EQ(result, callback_count);

    // Acceptable error codes: none or NOT_FOUND_ERROR (e.g., dir missing on CI)
    if (err_code != 0)
    {
        EXPECT_EQ(err_code.load(), static_cast<int>(StatusCodes::NOT_FOUND_ERROR));
    }

    serialOnError(nullptr);
}

// ---------------------------- Port listing helper ---------------------------
TEST(SerialGetPortsInfoTest, PrintAvailablePorts)
{
    auto print_callback = [](const char* port,
                             const char* path,
                             const char* /*manufacturer*/,
                             const char* /*serialNumber*/,
                             const char* /*pnpId*/,
                             const char* /*locationId*/,
                             const char* /*productId*/,
                             const char* /*vendorId*/) { std::cout << "  " << port << " (alias: " << path << ")\n"; };

    int result = serialGetPortsInfo(print_callback);
    EXPECT_GE(result, 0);
    if (result == 0)
    {
        std::cout << "\nNo serial devices found in /dev/serial/by-id\n";
    }
    else
    {
        std::cout << "\nAvailable serial ports (by-id):\n";
    }
}

// --------------------------- Stubbed no-op APIs ----------------------------
TEST(SerialStubbedFunctions, DoNotCrash)
{
    serialClearBufferIn(0);
    serialClearBufferOut(0);
    serialAbortRead(0);
    serialAbortWrite(0);
    SUCCEED(); // reached here without segfaults
}

TEST(SerialHelpers, ReadLine)
{
    SerialDevice dev;
    const std::string msg = "Hello World\n";
    dev.writeToDevice(msg);

    std::array<char, 64> read_buffer{};
    int num_read = serialReadLine(dev.handle, read_buffer.data(), static_cast<int>(read_buffer.size()), 2000);
    ASSERT_EQ(num_read, static_cast<int>(msg.size()));
    ASSERT_EQ(std::string_view(read_buffer.data(), num_read), msg);
}

TEST(SerialHelpers, ReadUntilToken)
{
    SerialDevice dev;
    const std::string payload = "ABC_OK";
    dev.writeToDevice(payload);

    std::array<char, 64> read_buffer{};
    constexpr std::string_view ok_token{"OK"};
    int num_read =
        serialReadUntilSequence(dev.handle, read_buffer.data(), static_cast<int>(read_buffer.size()), 2000, (void*)ok_token.data());
    ASSERT_EQ(num_read, static_cast<int>(payload.size()));
    ASSERT_EQ(std::string_view(read_buffer.data(), num_read), payload);
}

TEST(SerialHelpers, Statistics)
{
    SerialDevice dev;
    const std::string payload = "0123456789";

    // Transmit to device
    int written = serialWrite(dev.handle, payload.c_str(), static_cast<int>(payload.size()), 2000, 1);
    ASSERT_EQ(written, static_cast<int>(payload.size()));

    // Drain and read echo back
    serialDrain(dev.handle);

    std::array<char, 16> read_buffer{};
    int bytes_read = serialRead(dev.handle, read_buffer.data(), static_cast<int>(payload.size()), 2000, 1);
    ASSERT_EQ(bytes_read, static_cast<int>(payload.size()));

    ASSERT_EQ(serialOutBytesTotal(dev.handle), static_cast<int64_t>(payload.size()));
    ASSERT_EQ(serialInBytesTotal(dev.handle), static_cast<int64_t>(payload.size()));
}

TEST(SerialHelpers, Drain)
{
    SerialDevice dev;
    const std::string payload = "TEXT";
    int written = serialWriteLine(dev.handle, payload.c_str(), static_cast<int>(payload.size()), 2000);
    ASSERT_GT(written, 0);
    ASSERT_EQ(serialDrain(dev.handle), 1);
}
