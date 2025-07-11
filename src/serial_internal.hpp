#pragma once

#include <algorithm>
#include <atomic>
#include <cpp_core/error_callback.h>
#include <cpp_core/status_codes.h>
#include <cstdint>
#include <fcntl.h>
#include <mutex>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <utility>

namespace serial_internal
{
// ----------------------------------------------------------------------------
// Global callback function pointers (registered via serialSet*Callback APIs)
// ----------------------------------------------------------------------------
extern ErrorCallbackT g_error_callback;
extern void (*g_read_callback)(int);
extern void (*g_write_callback)(int);

// ----------------------------------------------------------------------------
// Helper to invoke the correct error callback (per-call overrides global)
// ----------------------------------------------------------------------------
inline void invokeError(int code, const char* message, ErrorCallbackT local)
{
    if (local != nullptr)
    {
        local(code, message);
    }
    else if (g_error_callback != nullptr)
    {
        g_error_callback(code, message);
    }
}

// ----------------------------------------------------------------------------
// Serial port handle that is returned to the user (opaque).
// ----------------------------------------------------------------------------
struct SerialPortHandle
{
    int fd{-1};         // File descriptor (POSIX)
    termios original{}; // Original TTY settings, restored on close.

    int64_t rx_total{0};
    int64_t tx_total{0};

    std::recursive_mutex mtx; // Protects concurrent read/write operations
    std::atomic<bool> abort_read{false};
    std::atomic<bool> abort_write{false};
};

// ----------------------------------------------------------------------------
// Utility: map integer baud rate to POSIX speed_t.
// ----------------------------------------------------------------------------
inline speed_t to_speed_t(int baud)
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
        return B9600; // reasonable fallback
    }
}

// ----------------------------------------------------------------------------
// Wait until the file descriptor is ready for read/write (select based).
// Returns >0 on ready, 0 on timeout, -1 on error.
// ----------------------------------------------------------------------------
inline int waitFdReady(int fd, int timeout_ms, bool want_write)
{
    timeout_ms = std::max(timeout_ms, 0);

    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);

    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    return select(fd + 1, want_write ? nullptr : &set, want_write ? &set : nullptr, nullptr, &tv);
}

} // namespace serial_internal
