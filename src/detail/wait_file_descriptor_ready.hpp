#pragma once

#include "consume_abort_flag.hpp"
#include "wait_result.hpp"

#include <algorithm>
#include <cerrno>
#include <poll.h>

namespace cpp_bindings_linux::detail
{
inline auto waitFileDescriptorReady(const std::shared_ptr<HandleState> &handle_state, int file_descriptor,
                                    int timeout_ms, Operation operation) -> WaitResult
{
    if (consumeAbortFlag(handle_state, operation))
    {
        return WaitResult::kAborted;
    }

    const short requested_events = operation == Operation::kRead ? POLLIN : POLLOUT;
    int remaining_timeout_ms = std::max(timeout_ms, 0);

    while (true)
    {
        pollfd poll_descriptor{
            .fd = file_descriptor,
            .events = requested_events,
            .revents = 0,
        };

        const int slice_timeout_ms = timeout_ms == 0 ? 0 : std::min(remaining_timeout_ms, 50);
        const int poll_status = poll(&poll_descriptor, 1, slice_timeout_ms);
        if (poll_status < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return WaitResult::kError;
        }

        if (poll_status > 0)
        {
            if ((poll_descriptor.revents & requested_events) != 0 ||
                (poll_descriptor.revents & (POLLERR | POLLHUP)) != 0)
            {
                return WaitResult::kReady;
            }
        }

        if (consumeAbortFlag(handle_state, operation))
        {
            return WaitResult::kAborted;
        }

        if (timeout_ms == 0)
        {
            return WaitResult::kTimedOut;
        }

        remaining_timeout_ms -= slice_timeout_ms;
        if (remaining_timeout_ms <= 0)
        {
            return WaitResult::kTimedOut;
        }
    }
}
} // namespace cpp_bindings_linux::detail
