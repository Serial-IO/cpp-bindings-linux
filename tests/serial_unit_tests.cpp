#include "serial.h"
#include "status_codes.h"

#include <atomic>
#include <cstring>
#include <gtest/gtest.h>
#include <iostream>

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
