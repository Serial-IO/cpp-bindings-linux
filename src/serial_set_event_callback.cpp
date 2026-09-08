#include <cpp_core/interface/serial_set_event_callback.h>

#include "detail/fail_errno.hpp"
#include "detail/handle_types.hpp"
#include "detail/is_serial_device_name.hpp"
#include "detail/status_value.hpp"

#include <cerrno>
#include <poll.h>
#include <string>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <system_error>
#include <thread>

namespace
{
using EventCallback = void (*)(cpp_core::PortEvent, const char *);
using cpp_bindings_linux::detail::UniqueFd;

struct EventListenerState
{
    UniqueFd inotify_fd;
    UniqueFd stop_fd;
    std::atomic<bool> stopped{false};

    void stop()
    {
        stopped.store(true, std::memory_order_release);
        const std::uint64_t value = 1;
        while (write(stop_fd.get(), &value, sizeof(value)) < 0 && errno == EINTR)
        {
        }
    }
};

void eventListenerLoop(const std::shared_ptr<EventListenerState> &state, EventCallback callback)
{
    alignas(inotify_event) char buffer[4096];
    while (!state->stopped.load(std::memory_order_acquire))
    {
        pollfd fds[2]{{state->inotify_fd.get(), POLLIN, 0}, {state->stop_fd.get(), POLLIN, 0}};
        const int ready = poll(fds, 2, -1);
        if (ready < 0 && errno == EINTR)
        {
            continue;
        }
        if (ready < 0 || fds[1].revents != 0 || (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
        {
            return;
        }
        if ((fds[0].revents & POLLIN) == 0)
        {
            continue;
        }
        const ssize_t bytes_read = read(state->inotify_fd.get(), buffer, sizeof(buffer));
        if (bytes_read <= 0)
        {
            continue;
        }
        const char *cursor = buffer;
        while (cursor < buffer + bytes_read && !state->stopped.load(std::memory_order_acquire))
        {
            const auto *event = reinterpret_cast<const inotify_event *>(cursor);
            if (event->len > 0 && cpp_bindings_linux::detail::isSerialDeviceName(event->name))
            {
                const std::string path = std::string("/dev/") + event->name;
                callback((event->mask & (IN_CREATE | IN_MOVED_TO)) != 0 ? cpp_core::PortEvent::kAttached
                                                                        : cpp_core::PortEvent::kDetached,
                         path.c_str());
            }
            cursor += sizeof(inotify_event) + event->len;
        }
    }
}

struct EventListener
{
    std::mutex mutex;
    std::shared_ptr<EventListenerState> state;
    std::thread thread;

    ~EventListener()
    {
        if (state)
        {
            state->stop();
        }
        if (thread.joinable())
        {
            thread.join();
        }
    }
};

EventListener g_event_listener;
} // namespace

MODULE_API auto serialSetEventCallback(EventCallback callback_fn, ErrorCallbackT error_callback) -> int
{
    // Build the replacement before changing the active event listener. A failed registration
    // leaves the current callback intact, and all descriptors are owned by its state.
    std::shared_ptr<EventListenerState> next;
    if (callback_fn != nullptr)
    {
        next = std::make_shared<EventListenerState>();
        next->inotify_fd = UniqueFd(inotify_init1(IN_CLOEXEC | IN_NONBLOCK));
        if (!next->inotify_fd.valid() ||
            inotify_add_watch(next->inotify_fd.get(), "/dev/", IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM) < 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Monitor::kMonitorError));
        }
        next->stop_fd = UniqueFd(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK));
        if (!next->stop_fd.valid())
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Monitor::kMonitorError));
        }
    }

    std::thread previous;
    try
    {
        std::lock_guard lock(g_event_listener.mutex);
        std::thread replacement;
        if (next)
        {
            replacement = std::thread(eventListenerLoop, next, callback_fn);
        }
        if (g_event_listener.state)
        {
            g_event_listener.state->stop();
        }
        previous = std::move(g_event_listener.thread);
        g_event_listener.state = std::move(next);
        g_event_listener.thread = std::move(replacement);
    }
    catch (const std::system_error &error)
    {
        return cpp_bindings_linux::detail::failMsg<int>(
            error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Monitor::kMonitorError),
            error.what());
    }

    // Join outside the lock so callbacks can register or clear themselves.
    if (previous.joinable())
    {
        if (previous.get_id() == std::this_thread::get_id())
        {
            previous.detach();
        }
        else
        {
            previous.join();
        }
    }
    return static_cast<int>(cpp_core::StatusCode::kSuccess);
}
