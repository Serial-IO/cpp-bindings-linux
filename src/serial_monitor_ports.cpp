#include <cpp_core/interface/serial_monitor_ports.h>

#include "detail/posix_fail_errno.hpp"
#include "detail/posix_is_serial_device_name.hpp"
#include "detail/posix_status_value.hpp"

#include <atomic>
#include <cstring>
#include <mutex>
#include <poll.h>
#include <string>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <thread>
#include <unistd.h>

namespace
{

std::mutex g_monitor_mutex;
std::thread g_monitor_thread;
int g_inotify_fd = -1;
int g_stop_fd = -1;
std::atomic<bool> g_running{false};

void monitorLoop(void (*callback)(int event, const char *port))
{
    (void)inotify_add_watch(g_inotify_fd, "/dev/", IN_CREATE | IN_DELETE);

    constexpr std::size_t kBufferLength = 4096;
    alignas(inotify_event) char buffer[kBufferLength];

    while (g_running.load(std::memory_order_relaxed))
    {
        pollfd fds[2]{};
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

        const ssize_t bytes_read = read(g_inotify_fd, buffer, kBufferLength);
        if (bytes_read <= 0)
        {
            continue;
        }

        const char *cursor = buffer;
        while (cursor < buffer + bytes_read)
        {
            const auto *event = reinterpret_cast<const inotify_event *>(cursor);
            if (event->len > 0 && cpp_bindings_linux::detail::isSerialDeviceName(event->name))
            {
                const std::string device_path = std::string("/dev/") + event->name;
                callback((event->mask & IN_CREATE) != 0 ? 1 : 0, device_path.c_str());
            }

            cursor += sizeof(inotify_event) + event->len;
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
        std::uint64_t value = 1;
        (void)write(g_stop_fd, &value, sizeof(value));
    }

    if (g_monitor_thread.joinable())
    {
        g_monitor_thread.join();
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
        std::lock_guard lock(g_monitor_mutex);
        stopMonitor();

        if (callback_fn == nullptr)
        {
            return static_cast<int>(cpp_core::StatusCode::kSuccess);
        }

        g_inotify_fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
        if (g_inotify_fd < 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Monitor::kMonitorError));
        }

        g_stop_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (g_stop_fd < 0)
        {
            close(g_inotify_fd);
            g_inotify_fd = -1;
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Monitor::kMonitorError));
        }

        g_running.store(true, std::memory_order_relaxed);
        g_monitor_thread = std::thread(monitorLoop, callback_fn);
        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
