#include "serial.h"

#include "status_codes.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
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
void (*error_callback)(int) = nullptr;
void (*read_callback)(int) = nullptr;
void (*write_callback)(int) = nullptr;

// -----------------------------------------------------------------------------
// Internal helpers & types
// -----------------------------------------------------------------------------
namespace
{

struct SerialPortHandle
{
    int fd;
    termios original; // keep original settings so we can restore on close

    // --- extensions ---
    int64_t rx_total{0}; // bytes received so far
    int64_t tx_total{0}; // bytes transmitted so far

    bool has_peek{false};
    char peek_char{0};

    // Abort flags (set from other threads)
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

inline void invokeError(int code)
{
    if (error_callback != nullptr)
    {
        error_callback(code);
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
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR));
        return 0;
    }

    auto port_name = std::string_view{static_cast<const char*>(port)};
    int fd = open(port_name.data(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR));
        return 0;
    }

    auto* handle = new SerialPortHandle{.fd = fd, .original = {}};

    termios tty{};
    if (tcgetattr(fd, &tty) != 0)
    {
        invokeError(std::to_underlying(StatusCodes::GET_STATE_ERROR));
        close(fd);
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

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        invokeError(std::to_underlying(StatusCodes::SET_STATE_ERROR));
        close(fd);
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

    tcsetattr(handle->fd, TCSANOW, &handle->original); // restore
    if (close(handle->fd) != 0)
    {
        invokeError(std::to_underlying(StatusCodes::CLOSE_HANDLE_ERROR));
    }
    delete handle;
}

static int waitFdReady(int fd, int timeoutMs, bool wantWrite)
{
    timeoutMs = std::max(timeoutMs, 0);

    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);

    timeval tv{};
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int res = select(fd + 1, wantWrite ? nullptr : &set, wantWrite ? &set : nullptr, nullptr, &tv);
    return res; // 0 timeout, -1 error, >0 ready
}

int serialRead(int64_t handlePtr, void* buffer, int bufferSize, int timeout, int /*multiplier*/)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR));
        return 0;
    }

    // Abort check
    if (handle->abort_read.exchange(false))
    {
        return 0;
    }

    int total_copied = 0;

    // First deliver byte from internal peek buffer if present
    if (handle->has_peek && bufferSize > 0)
    {
        static_cast<char*>(buffer)[0] = handle->peek_char;
        handle->has_peek = false;
        handle->rx_total += 1;
        total_copied = 1;
        buffer = static_cast<char*>(buffer) + 1;
        bufferSize -= 1;
        if (bufferSize == 0)
        {
            if (read_callback != nullptr)
            {
                read_callback(total_copied);
            }
            return total_copied;
        }
    }

    if (waitFdReady(handle->fd, timeout, false) <= 0)
    {
        return total_copied; // return what we may have already copied (could be 0)
    }

    ssize_t n = read(handle->fd, buffer, bufferSize);
    if (n < 0)
    {
        invokeError(std::to_underlying(StatusCodes::READ_ERROR));
        return total_copied;
    }

    if (n > 0)
    {
        handle->rx_total += n;
    }

    total_copied += static_cast<int>(n);

    if (read_callback != nullptr)
    {
        read_callback(total_copied);
    }
    return total_copied;
}

int serialWrite(int64_t handlePtr, const void* buffer, int bufferSize, int timeout, int /*multiplier*/)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR));
        return 0;
    }

    // Abort check
    if (handle->abort_write.exchange(false))
    {
        return 0;
    }

    if (waitFdReady(handle->fd, timeout, true) <= 0)
    {
        return 0; // timeout or error
    }

    ssize_t n = write(handle->fd, buffer, bufferSize);
    if (n < 0)
    {
        invokeError(std::to_underlying(StatusCodes::WRITE_ERROR));
        return 0;
    }

    if (n > 0)
    {
        handle->tx_total += n;
    }

    if (write_callback != nullptr)
    {
        write_callback(static_cast<int>(n));
    }
    return static_cast<int>(n);
}

int serialReadUntil(int64_t handlePtr, void* buffer, int bufferSize, int timeout, int /*multiplier*/, void* untilCharPtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR));
        return 0;
    }

    char until_char = *static_cast<char*>(untilCharPtr);
    int total = 0;
    auto* buf = static_cast<char*>(buffer);

    while (total < bufferSize)
    {
        int res = serialRead(handlePtr, buf + total, 1, timeout, 1);
        if (res <= 0)
        {
            break; // timeout or error
        }
        if (buf[total] == until_char)
        {
            total += 1;
            break;
        }
        total += res;
    }

    if (read_callback != nullptr)
    {
        read_callback(total);
    }
    return total;
}

int serialGetPortsInfo(void* buffer, int bufferSize, void* separatorPtr)
{
    auto sep = std::string_view{static_cast<const char*>(separatorPtr)};
    std::string result;

    namespace fs = std::filesystem;

    const fs::path by_id_dir{"/dev/serial/by-id"};
    if (!fs::exists(by_id_dir) || !fs::is_directory(by_id_dir))
    {
        invokeError(std::to_underlying(StatusCodes::NOT_FOUND_ERROR));
        return 0;
    }

    try
    {
        for (const auto& entry : fs::directory_iterator{by_id_dir})
        {
            if (!entry.is_symlink())
            {
                continue;
            }

            std::error_code ec;
            fs::path canonical = fs::canonical(entry.path(), ec);
            if (ec)
            {
                continue; // skip entries we cannot resolve
            }

            result += canonical.string();
            result += sep;
        }
    }
    catch (const fs::filesystem_error&)
    {
        invokeError(std::to_underlying(StatusCodes::NOT_FOUND_ERROR));
        return 0;
    }

    if (!result.empty())
    {
        // Remove the trailing separator
        result.erase(result.size() - sep.size());
    }

    if (static_cast<int>(result.size()) + 1 > bufferSize)
    {
        invokeError(std::to_underlying(StatusCodes::BUFFER_ERROR));
        return 0;
    }

    std::memcpy(buffer, result.c_str(), result.size() + 1);
    return result.empty() ? 0 : 1; // number of ports not easily counted here
}

// -----------------------------------------------------------------------------
// Buffer & abort helpers implementations
// -----------------------------------------------------------------------------

void serialClearBufferIn(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        return;
    }
    tcflush(handle->fd, TCIFLUSH);
    // reset peek buffer
    handle->has_peek = false;
}

void serialClearBufferOut(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        return;
    }
    tcflush(handle->fd, TCOFLUSH);
}

void serialAbortRead(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        return;
    }
    handle->abort_read = true;
}

void serialAbortWrite(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        return;
    }
    handle->abort_write = true;
}

// -----------------------------------------------------------------------------

// Callback registration
void serialOnError(void (*func)(int code))
{
    error_callback = func;
}
void serialOnRead(void (*func)(int bytes))
{
    read_callback = func;
}
void serialOnWrite(void (*func)(int bytes))
{
    write_callback = func;
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
    char nl = '\n';
    int w_nl = serialWrite(handlePtr, &nl, 1, timeout, 1);
    if (w_nl != 1)
    {
        return written; // newline failed, but payload written
    }
    return written + 1;
}

int serialReadUntilToken(int64_t handlePtr, void* buffer, int bufferSize, int timeout, void* tokenPtr)
{
    auto* token_cstr = static_cast<const char*>(tokenPtr);
    if (token_cstr == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR));
        return 0;
    }
    std::string token{token_cstr};
    int token_len = static_cast<int>(token.size());
    if (token_len == 0 || bufferSize < token_len)
    {
        return 0;
    }

    auto* buf = static_cast<char*>(buffer);
    int total = 0;
    int matched = 0; // how many chars of token matched so far

    while (total < bufferSize)
    {
        int res = serialRead(handlePtr, buf + total, 1, timeout, 1);
        if (res <= 0)
        {
            break; // timeout or error
        }

        char c = buf[total];
        total += 1;

        if (c == token[matched])
        {
            matched += 1;
            if (matched == token_len)
            {
                break; // token fully matched
            }
        }
        else
        {
            matched = (c == token[0]) ? 1 : 0; // restart match search
        }
    }

    if (read_callback != nullptr)
    {
        read_callback(total);
    }
    return total;
}

int serialReadFrame(int64_t handlePtr, void* buffer, int bufferSize, int timeout, char startByte, char endByte)
{
    auto* buf = static_cast<char*>(buffer);
    int total = 0;
    bool in_frame = false;

    while (total < bufferSize)
    {
        char byte;
        int res = serialRead(handlePtr, &byte, 1, timeout, 1);
        if (res <= 0)
        {
            break; // timeout
        }

        if (!in_frame)
        {
            if (byte == startByte)
            {
                in_frame = true;
                buf[total++] = byte;
            }
            continue; // ignore bytes until start byte detected
        }
        else
        {
            buf[total++] = byte;
            if (byte == endByte)
            {
                break; // frame finished
            }
        }
    }

    return total;
}

int64_t serialGetRxBytes(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->rx_total;
}

int64_t serialGetTxBytes(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->tx_total;
}

int serialPeek(int64_t handlePtr, void* outByte, int timeout)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr || outByte == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR));
        return 0;
    }

    if (handle->has_peek)
    {
        *static_cast<char*>(outByte) = handle->peek_char;
        return 1;
    }

    char c;
    int res = serialRead(handlePtr, &c, 1, timeout, 1);
    if (res <= 0)
    {
        return 0; // nothing available
    }

    // Store into peek buffer and undo stats increment
    handle->peek_char = c;
    handle->has_peek = true;
    if (handle->rx_total > 0)
    {
        handle->rx_total -= 1; // don't account peek
    }

    *static_cast<char*>(outByte) = c;
    return 1;
}

int serialDrain(int64_t handlePtr)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handlePtr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(StatusCodes::INVALID_HANDLE_ERROR));
        return 0;
    }
    return (tcdrain(handle->fd) == 0) ? 1 : 0;
}
