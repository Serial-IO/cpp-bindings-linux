#include "serial.h"

#include "status_codes.h"

#include <algorithm>
#include <atomic>
#include <fcntl.h>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <utility>

// -----------------------------------------------------------------------------
// Global callback function pointers (default nullptr)
// -----------------------------------------------------------------------------
void (*on_error_callback)(int errorCode, const char* message) = nullptr;
void (*on_read_callback)(int bytes) = nullptr;
void (*on_write_callback)(int bytes) = nullptr;

// -----------------------------------------------------------------------------
// Internal helpers & types
// -----------------------------------------------------------------------------
namespace
{

struct SerialPortHandle
{
    int file_descriptor;
    termios original; // keep original settings so we can restore on close

    int64_t rx_total{0}; // bytes received so far
    int64_t tx_total{0}; // bytes transmitted so far

    std::atomic<bool> abort_read{false};
    std::atomic<bool> abort_write{false};
};

// Map integer baudrate to POSIX speed_t. Only common rates are supported.
auto to_speed_t(int baud) -> speed_t
{
    switch (baud)
    {
    case 0:
        return B0;
    case 50:
        return B50;
    case 75:
        return B75;
    case 110:
        return B110;
    case 134:
        return B134;
    case 150:
        return B150;
    case 200:
        return B200;
    case 300:
        return B300;
    case 600:
        return B600;
    case 1200:
        return B1200;
    case 1800:
        return B1800;
    case 2400:
        return B2400;
    case 4800:
        return B4800;
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
#ifdef B460800
    case 460800:
        return B460800;
#endif
#ifdef B921600
    case 921600:
        return B921600;
#endif
    default:
        return B9600; // fallback
    }
}

inline void invokeError(int code, const char* message)
{
    if (on_error_callback != nullptr)
    {
        on_error_callback(code, message);
    }
}

} // namespace

// -----------------------------------------------------------------------------
// Public API implementation
// -----------------------------------------------------------------------------

intptr_t serialOpen(void* port, int baudrate, int dataBits, int parity, int stopBits)
{
    if (port == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR), "serialOpen: Invalid handle");
        return 0;
    }

    auto port_name = std::string_view{static_cast<const char*>(port)};
    int device_descriptor = open(port_name.data(), O_RDWR | O_NOCTTY | O_SYNC);
    if (device_descriptor < 0)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR), "serialOpen: Failed to open serial port");
        return 0;
    }

    auto* handle = new SerialPortHandle{.file_descriptor = device_descriptor, .original = {}};

    termios tty{};
    if (tcgetattr(device_descriptor, &tty) != 0)
    {
        invokeError(std::to_underlying(StatusCodes::GET_STATE_ERROR), "serialOpen: Failed to get serial attributes");
        close(device_descriptor);
        delete handle;
        return 0;
    }
    handle->original = tty; // save original

    // Basic flags: local connection, enable receiver
    tty.c_cflag |= (CLOCAL | CREAD);

    // Baudrate
    const speed_t speed = to_speed_t(baudrate);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    // Data bits
    tty.c_cflag &= ~CSIZE;
    switch (dataBits)
    {
    case 5:
        tty.c_cflag |= CS5;
        break;
    case 6:
        tty.c_cflag |= CS6;
        break;
    case 7:
        tty.c_cflag |= CS7;
        break;
    default:
        tty.c_cflag |= CS8;
        break;
    }

    // Parity
    if (parity == 0)
    {
        tty.c_cflag &= ~PARENB;
    }
    else
    {
        tty.c_cflag |= PARENB;
        if (parity == 1)
        {
            tty.c_cflag &= ~PARODD; // even
        }
        else
        {
            tty.c_cflag |= PARODD; // odd
        }
    }

    // Stop bits
    if (stopBits == 2)
    {
        tty.c_cflag |= CSTOPB;
    }
    else
    {
        tty.c_cflag &= ~CSTOPB;
    }

    // Raw mode (no echo/processing)
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;

    tty.c_cc[VMIN] = 0;   // non-blocking by default
    tty.c_cc[VTIME] = 10; // 1s read timeout

    if (tcsetattr(device_descriptor, TCSANOW, &tty) != 0)
    {
        invokeError(std::to_underlying(StatusCodes::SET_STATE_ERROR), "serialOpen: Failed to set serial attributes");
        close(device_descriptor);
        delete handle;
        return 0;
    }

    return reinterpret_cast<intptr_t>(handle);
}

void serialClose(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        return;
    }

    tcsetattr(handle->file_descriptor, TCSANOW, &handle->original); // restore
    if (close(handle->file_descriptor) != 0)
    {
        invokeError(std::to_underlying(StatusCodes::CLOSE_HANDLE_ERROR), "serialClose: Failed to close serial port");
    }
    delete handle;
}

static int waitFdReady(int fileDescriptor, int timeoutMs, bool wantWrite)
{
    timeoutMs = std::max(timeoutMs, 0);

    fd_set descriptor_set;
    FD_ZERO(&descriptor_set);
    FD_SET(fileDescriptor, &descriptor_set);

    timeval wait_time{};
    wait_time.tv_sec = timeoutMs / 1000;
    wait_time.tv_usec = (timeoutMs % 1000) * 1000;

    int ready_result =
        select(fileDescriptor + 1, wantWrite ? nullptr : &descriptor_set, wantWrite ? &descriptor_set : nullptr, nullptr, &wait_time);
    return ready_result; // 0 timeout, -1 error, >0 ready
}

int serialRead(int64_t handlePtr, void* buffer, int bufferSize, int timeout, int /*multiplier*/)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR), "serialRead: Invalid handle");
        return 0;
    }

    // Abort check
    if (handle->abort_read.exchange(false))
    {
        return 0;
    }

    int total_copied = 0;

    if (waitFdReady(handle->file_descriptor, timeout, false) <= 0)
    {
        return total_copied; // return what we may have already copied (could be 0)
    }

    ssize_t bytes_read_system = read(handle->file_descriptor, buffer, bufferSize);
    if (bytes_read_system < 0)
    {
        invokeError(std::to_underlying(StatusCodes::READ_ERROR), "serialRead: Read error");
        return total_copied;
    }

    if (bytes_read_system > 0)
    {
        handle->rx_total += bytes_read_system;
    }

    total_copied += static_cast<int>(bytes_read_system);

    if (on_read_callback != nullptr)
    {
        on_read_callback(total_copied);
    }
    return total_copied;
}

int serialWrite(int64_t handlePtr, const void* buffer, int bufferSize, int timeout, int /*multiplier*/)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR), "serialWrite: Invalid handle");
        return 0;
    }

    // Abort check
    if (handle->abort_write.exchange(false))
    {
        return 0;
    }

    if (waitFdReady(handle->file_descriptor, timeout, true) <= 0)
    {
        return 0; // timeout or error
    }

    ssize_t bytes_written_system = write(handle->file_descriptor, buffer, bufferSize);
    if (bytes_written_system < 0)
    {
        invokeError(std::to_underlying(StatusCodes::WRITE_ERROR), "serialWrite: Write error");
        return 0;
    }

    if (bytes_written_system > 0)
    {
        handle->tx_total += bytes_written_system;
    }

    if (on_write_callback != nullptr)
    {
        on_write_callback(static_cast<int>(bytes_written_system));
    }
    return static_cast<int>(bytes_written_system);
}

int serialReadUntil(int64_t handlePtr, void* buffer, int bufferSize, int timeout, int /*multiplier*/, void* untilCharPtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR), "serialReadUntil: Invalid handle");
        return 0;
    }

    char until_character = *static_cast<char*>(untilCharPtr);
    int total = 0;
    auto* char_buffer = static_cast<char*>(buffer);

    while (total < bufferSize)
    {
        int read_result = serialRead(handlePtr, char_buffer + total, 1, timeout, 1);
        if (read_result <= 0)
        {
            break; // timeout or error
        }
        if (char_buffer[total] == until_character)
        {
            total += 1;
            break;
        }
        total += read_result;
    }

    if (on_read_callback != nullptr)
    {
        on_read_callback(total);
    }
    return total;
}

// -----------------------------------------------------------------------------
// Buffer & abort helpers implementations
// -----------------------------------------------------------------------------

void serialClearBufferIn(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR), "serialClearBufferIn: Invalid handle");
        return;
    }
    tcflush(handle->file_descriptor, TCIFLUSH);
}

void serialClearBufferOut(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR), "serialClearBufferOut: Invalid handle");
        return;
    }
    tcflush(handle->file_descriptor, TCOFLUSH);
}

void serialAbortRead(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR), "serialAbortRead: Invalid handle");
        return;
    }
    handle->abort_read = true;
}

void serialAbortWrite(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR), "serialAbortWrite: Invalid handle");
        return;
    }
    handle->abort_write = true;
}

// -----------------------------------------------------------------------------

// Callback registration
void serialOnError(void (*func)(int code, const char* message))
{
    on_error_callback = func;
}
void serialOnRead(void (*func)(int bytes))
{
    on_read_callback = func;
}
void serialOnWrite(void (*func)(int bytes))
{
    on_write_callback = func;
}

// -----------------------------------------------------------------------------
// Extended helper APIs (read line, token, frame, statistics, etc.)
// -----------------------------------------------------------------------------

int serialReadLine(int64_t handlePtr, void* buffer, int bufferSize, int timeout)
{
    char newline = '\n';
    return serialReadUntil(handlePtr, buffer, bufferSize, timeout, 1, &newline);
}

int serialWriteLine(int64_t handlePtr, const void* buffer, int bufferSize, int timeout)
{
    // First write the payload
    int written = serialWrite(handlePtr, buffer, bufferSize, timeout, 1);
    if (written != bufferSize)
    {
        return written; // error path, propagate
    }
    // Append newline (\n)
    char new_line_char = '\n';
    int newline_result = serialWrite(handlePtr, &new_line_char, 1, timeout, 1);
    if (newline_result != 1)
    {
        return written; // newline failed, but payload written
    }
    return written + 1;
}

int serialReadUntilSequence(int64_t handlePtr, void* buffer, int bufferSize, int timeout, void* sequencePtr)
{
    const auto* sequence_cstr = static_cast<const char*>(sequencePtr);
    if (sequence_cstr == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR), "serialReadUntilSequence: Invalid sequence");
        return 0;
    }
    std::string sequence{sequence_cstr};
    int sequence_len = static_cast<int>(sequence.size());
    if (sequence_len == 0 || bufferSize < sequence_len)
    {
        return 0;
    }

    auto* char_buffer = static_cast<char*>(buffer);
    int total = 0;
    int matched = 0; // how many chars of sequence matched so far

    while (total < bufferSize)
    {
        int read_result = serialRead(handlePtr, char_buffer + total, 1, timeout, 1);
        if (read_result <= 0)
        {
            break; // timeout or error
        }

        char current_char = char_buffer[total];
        total += 1;

        if (current_char == sequence[matched])
        {
            matched += 1;
            if (matched == sequence_len)
            {
                break; // sequence fully matched
            }
        }
        else
        {
            matched = (current_char == sequence[0]) ? 1 : 0; // restart match search
        }
    }

    if (on_read_callback != nullptr)
    {
        on_read_callback(total);
    }
    return total;
}

int serialReadFrame(int64_t handlePtr, void* buffer, int bufferSize, int timeout, char startByte, char endByte)
{
    auto* char_buffer = static_cast<char*>(buffer);
    int total = 0;
    bool in_frame = false;

    while (total < bufferSize)
    {
        char current_byte;
        int read_result = serialRead(handlePtr, &current_byte, 1, timeout, 1);
        if (read_result <= 0)
        {
            break; // timeout
        }

        if (!in_frame)
        {
            if (current_byte == startByte)
            {
                in_frame = true;
                char_buffer[total++] = current_byte;
            }
            continue; // ignore bytes until start byte detected
        }

        char_buffer[total++] = current_byte;
        if (current_byte == endByte)
        {
            break; // frame finished
        }
    }

    return total;
}

int64_t serialInBytesTotal(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->rx_total;
}

int64_t serialOutBytesTotal(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->tx_total;
}

// Bytes currently queued in the driver/OS buffers --------------------------------
int serialInBytesWaiting(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR), "serialInBytesWaiting: Invalid handle");
        return 0;
    }

    int bytes_available = 0;
    if (ioctl(handle->file_descriptor, FIONREAD, &bytes_available) == -1)
    {
        invokeError(std::to_underlying(StatusCodes::GET_STATE_ERROR), "serialInBytesWaiting: Failed to get state");
        return 0;
    }
    return bytes_available;
}

int serialOutBytesWaiting(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR), "serialOutBytesWaiting: Invalid handle");
        return 0;
    }

    int bytes_queued = 0;
#ifdef TIOCOUTQ
    if (ioctl(handle->file_descriptor, TIOCOUTQ, &bytes_queued) == -1)
    {
        invokeError(std::to_underlying(StatusCodes::GET_STATE_ERROR), "serialOutBytesWaiting: Failed to get state");
        return 0;
    }
#else
    // TIOCOUTQ not available on this platform – fallback: 0
    bytes_queued = 0;
#endif
    return bytes_queued;
}

int serialDrain(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR), "serialDrain: Invalid handle");
        return 0;
    }
    return (tcdrain(handle->file_descriptor) == 0) ? 1 : 0;
}
