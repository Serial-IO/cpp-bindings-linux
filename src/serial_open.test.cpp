#include <cpp_core/interface/serial_open.h>
#include <cpp_core/status_codes.h>

#include <array>
#include <limits>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "test_helpers/error_capture.hpp"

// ============================================================================
// Tests for serialOpen
// ============================================================================

class SerialOpenTest : public ::testing::Test
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

TEST_F(SerialOpenTest, NullPortParameter)
{
    intptr_t result = serialOpen(nullptr, 9600, 8, 0, 1, error_callback);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kNotFoundError));
    EXPECT_NE(error_capture.last_message.find("nullptr"), std::string::npos);
}

TEST_F(SerialOpenTest, BaudrateTooLow)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 100, 8, 0, 1, error_callback);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
    EXPECT_NE(error_capture.last_message.find("baudrate"), std::string::npos);
}

TEST_F(SerialOpenTest, BaudrateTooLowBoundary)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 299, 8, 0, 1, error_callback);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, BaudrateBoundaryValid)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 300, 8, 0, 1, error_callback);

    // /dev/null is not a real serial port, but should pass baudrate validation
    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, DataBitsTooLow)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 4, 0, 1, error_callback);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
    EXPECT_NE(error_capture.last_message.find("data bits"), std::string::npos);
}

TEST_F(SerialOpenTest, DataBitsTooHigh)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 9, 0, 1, error_callback);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidDataBits5)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 5, 0, 1, error_callback);

    // Should pass data bits validation
    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidDataBits6)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 6, 0, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidDataBits7)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 7, 0, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidDataBits8)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, InvalidParity)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 5, 1, error_callback);

    // Invalid parity should return an error
    EXPECT_LT(result, 0);
}

TEST_F(SerialOpenTest, ValidParityNone)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidParityEven)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 1, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidParityOdd)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 2, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, InvalidStopBits)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 3, error_callback);

    // Invalid stop bits should return an error
    EXPECT_LT(result, 0);
}

TEST_F(SerialOpenTest, ValidStopBits0)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 0, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidStopBits1)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 1, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, ValidStopBits2)
{
    const char *port = "/dev/null";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 2, error_callback);

    EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError));
}

TEST_F(SerialOpenTest, NonExistentPort)
{
    const char *port = "/dev/ttyNONEXISTENT99999";
    intptr_t result = serialOpen(const_cast<void *>(static_cast<const void *>(port)), 9600, 8, 0, 1, error_callback);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kNotFoundError));
}

TEST_F(SerialOpenTest, VariousBaudrates)
{
    const char *port = "/dev/null";

    // Test common baudrates
    const std::array<int, 11> baudrates = {300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800};

    for (int baudrate : baudrates)
    {
        intptr_t result =
            serialOpen(const_cast<void *>(static_cast<const void *>(port)), baudrate, 8, 0, 1, error_callback);
        EXPECT_NE(result, static_cast<intptr_t>(cpp_core::StatusCodes::kSetStateError))
            << "Baudrate " << baudrate << " should be valid";
    }
}

TEST_F(SerialOpenTest, NoErrorCallbackNullPort)
{
    intptr_t result = serialOpen(nullptr, 9600, 8, 0, 1, nullptr);

    EXPECT_EQ(result, static_cast<intptr_t>(cpp_core::StatusCodes::kNotFoundError));
}
