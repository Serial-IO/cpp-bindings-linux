#include <cpp_core/interface/serial_monitor_ports.h>
#include <cpp_core/status_codes.h>

#include "detail/posix_helpers.hpp"

#include <atomic>
#include <cstring>
#include <mutex>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <thread>
#include <unistd.h>

namespace
{

std::mutex g_mutex;
std::thread g_thread;
int g_inotify_fd = -1;
int g_stop_fd = -1;
std::atomic<bool> g_running{false};

auto isSerialDevice(const char *name) -> bool
{
    return std::strncmp(name, "ttyUSB", 6) == 0 || std::strncmp(name, "ttyACM", 6) == 0 ||
           std::strncmp(name, "ttyS", 4) == 0 || std::strncmp(name, "ttyAMA", 6) == 0;
}

void monitorLoop(void (*callback)(int event, const char *port))
{
    inotify_add_watch(g_inotify_fd, "/dev/", IN_CREATE | IN_DELETE);

    constexpr size_t kBufLen = 4096;
    alignas(struct inotify_event) char buf[kBufLen];

    while (g_running.load(std::memory_order_relaxed))
    {
        struct pollfd fds[2] = {};
        fds[0].fd = g_inotify_fd;
        fds[0].events = POLLIN;
        fds[1].fd = g_stop_fd;
        fds[1].events = POLLIN;

        const int ready = poll(fds, 2, 1000);
        if (ready <= 0)
        {
            continue;
        }

        if ((fds[1].revents & POLLIN) != 0)
        {
            break;
        }

        if ((fds[0].revents & POLLIN) == 0)
        {
            continue;
        }

        const ssize_t len = read(g_inotify_fd, buf, kBufLen);
        if (len <= 0)
        {
            continue;
        }

        const char *ptr = buf;
        while (ptr < buf + len)
        {
            const auto *event = reinterpret_cast<const struct inotify_event *>(ptr);
            if (event->len > 0 && isSerialDevice(event->name))
            {
                std::string dev_path = std::string("/dev/") + event->name;
                const int ev = (event->mask & IN_CREATE) ? 1 : 0;
                callback(ev, dev_path.c_str());
            }
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
}

void stopMonitor()
{
    if (!g_running.load(std::memory_order_relaxed))
    {
        return;
    }

    g_running.store(false, std::memory_order_relaxed);

    if (g_stop_fd >= 0)
    {
        uint64_t val = 1;
        (void)write(g_stop_fd, &val, sizeof(val));
    }

    if (g_thread.joinable())
    {
        g_thread.join();
    }

    if (g_inotify_fd >= 0)
    {
        close(g_inotify_fd);
        g_inotify_fd = -1;
    }
    if (g_stop_fd >= 0)
    {
        close(g_stop_fd);
        g_stop_fd = -1;
    }
}

} // namespace

extern "C"
{

    MODULE_API auto serialMonitorPorts(void (*callback_fn)(int event, const char *port),
                                       ErrorCallbackT error_callback) -> int
    {
        std::lock_guard lock(g_mutex);

        stopMonitor();

        if (callback_fn == nullptr)
        {
            return static_cast<int>(cpp_core::StatusCodes::kSuccess);
        }

        g_inotify_fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
        if (g_inotify_fd < 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kMonitorError);
        }

        g_stop_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (g_stop_fd < 0)
        {
            close(g_inotify_fd);
            g_inotify_fd = -1;
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kMonitorError);
        }

        g_running.store(true, std::memory_order_relaxed);
        g_thread = std::thread(monitorLoop, callback_fn);

        return static_cast<int>(cpp_core::StatusCodes::kSuccess);
    }

} // extern "C"
