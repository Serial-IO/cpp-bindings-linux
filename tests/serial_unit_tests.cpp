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
#include <vector>

namespace
{
// Helper storage for callback tests
std::atomic<int>* g_err_ptr = nullptr;

void errorCallback(int code)
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

    void writeToDevice(std::string_view data)
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
TEST(SerialGetPortsInfoTest, BufferTooSmallTriggersError)
{
    char sep[] = ";";
    char buffer[4];
    std::atomic<int> err_code{0};

    g_err_ptr = &err_code;
    serialOnError(errorCallback);

    int res = serialGetPortsInfo(buffer, sizeof(buffer), sep);
    EXPECT_EQ(res, 0); // function indicates failure via 0
    EXPECT_EQ(err_code.load(), static_cast<int>(StatusCodes::BUFFER_ERROR));

    serialOnError(nullptr);
}

TEST(SerialGetPortsInfoTest, LargeBufferReturnsZeroOrOne)
{
    char sep[] = ";";
    char buffer[4096] = {0};

    std::atomic<int> err_code{0};
    g_err_ptr = &err_code;
    serialOnError(errorCallback);

    int res = serialGetPortsInfo(buffer, sizeof(buffer), sep);
    EXPECT_GE(res, 0);
    // res is 0 (no ports) or 1 (ports found)
    EXPECT_LE(res, 1);
    // Acceptable error codes: none or NOT_FOUND_ERROR (e.g., dir missing)
    if (err_code != 0)
    {
        EXPECT_EQ(err_code.load(), static_cast<int>(StatusCodes::NOT_FOUND_ERROR));
    }

    serialOnError(nullptr);
}

// ---------------------------- Port listing helper ---------------------------
TEST(SerialGetPortsInfoTest, PrintAvailablePorts)
{
    char sep[] = ";";
    char buffer[4096] = {0};

    int res = serialGetPortsInfo(buffer, sizeof(buffer), sep);
    EXPECT_GE(res, 0);

    std::string ports_str(buffer);
    if (!ports_str.empty())
    {
        std::cout << "\nAvailable serial ports (by-id):\n";
        size_t start = 0;
        while (true)
        {
            size_t pos = ports_str.find(sep, start);
            std::string token = ports_str.substr(start, pos - start);
            std::cout << "  " << token << "\n";
            if (pos == std::string::npos)
            {
                break;
            }
            start = pos + std::strlen(sep);
        }
    }
    else
    {
        std::cout << "\nNo serial devices found in /dev/serial/by-id\n";
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

    char buf[64] = {0};
    int n = serialReadLine(dev.handle, buf, sizeof(buf), 2000);
    ASSERT_EQ(n, static_cast<int>(msg.size()));
    ASSERT_EQ(std::string_view(buf, n), msg);
}

TEST(SerialHelpers, ReadUntilToken)
{
    SerialDevice dev;
    const std::string payload = "ABC_OK";
    dev.writeToDevice(payload);

    char buf[64] = {0};
    const char token[] = "OK";
    int n = serialReadUntilToken(dev.handle, buf, sizeof(buf), 2000, (void*)token);
    ASSERT_EQ(n, static_cast<int>(payload.size()));
    ASSERT_EQ(std::string_view(buf, n), payload);
}

TEST(SerialHelpers, Peek)
{
    SerialDevice dev;
    const std::string payload = "XYZ";
    dev.writeToDevice(payload);

    char first = 0;
    int res = serialPeek(dev.handle, &first, 2000);
    ASSERT_EQ(res, 1);
    ASSERT_EQ(first, 'X');

    char buf[4] = {0};
    int n = serialRead(dev.handle, buf, 3, 2000, 1);
    ASSERT_EQ(n, 3);
    ASSERT_EQ(std::string_view(buf, 3), payload);
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

    char buf[16] = {0};
    int read_bytes = serialRead(dev.handle, buf, static_cast<int>(payload.size()), 2000, 1);
    ASSERT_EQ(read_bytes, static_cast<int>(payload.size()));

    ASSERT_EQ(serialGetTxBytes(dev.handle), static_cast<int64_t>(payload.size()));
    ASSERT_EQ(serialGetRxBytes(dev.handle), static_cast<int64_t>(payload.size()));
}

TEST(SerialHelpers, Drain)
{
    SerialDevice dev;
    const std::string payload = "TEXT";
    int written = serialWriteLine(dev.handle, payload.c_str(), static_cast<int>(payload.size()), 2000);
    ASSERT_GT(written, 0);
    ASSERT_EQ(serialDrain(dev.handle), 1);
}
