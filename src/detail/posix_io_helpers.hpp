#pragma once

#include "posix_handle_helpers.hpp"

#include <algorithm>
#include <cstring>
#include <poll.h>
#include <unistd.h>

namespace cpp_bindings_linux::detail
{
enum class WaitResult
{
    kReady,
    kTimedOut,
    kAborted,
    kError,
};

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

inline auto multiplierTimeout(int timeout_ms, int multiplier) -> int
{
    if (multiplier <= 0)
    {
        return 0;
    }

    return cpp_core::clampTimeout(timeout_ms) * multiplier;
}

inline auto matchesSuffix(const unsigned char *buffer, int buffer_size, const unsigned char *terminator,
                          int terminator_size) -> bool
{
    if (terminator == nullptr || terminator_size <= 0 || buffer_size < terminator_size)
    {
        return false;
    }

    return std::memcmp(buffer + (buffer_size - terminator_size), terminator,
                       static_cast<std::size_t>(terminator_size)) == 0;
}

inline auto readImpl(int64_t handle, void *buffer, int buffer_size, int timeout_ms, int multiplier,
                     const unsigned char *terminator, int terminator_size, ErrorCallbackT error_callback) -> int
{
    const auto callback = effectiveErrorCallback(error_callback);
    const auto buffer_status = cpp_core::validateBuffer<int>(buffer, buffer_size, callback);
    if (buffer_status < 0)
    {
        return buffer_status;
    }

    if (terminator_size > 0 && terminator == nullptr)
    {
        return failMsg<int>(error_callback, statusValue(StatusCode::Io::kBufferError), "Invalid terminator");
    }

    HandleContext handle_context;
    const auto handle_status = acquireHandleContext<int>(handle, callback, &handle_context);
    if (handle_status < 0)
    {
        return handle_status;
    }

    auto *output = static_cast<unsigned char *>(buffer);
    const bool read_single_bytes = terminator_size > 0;
    int total_read = 0;

    while (total_read < buffer_size)
    {
        const int current_timeout_ms =
            total_read == 0 ? cpp_core::clampTimeout(timeout_ms) : multiplierTimeout(timeout_ms, multiplier);
        switch (waitFileDescriptorReady(handle_context.state, handle_context.file_descriptor, current_timeout_ms,
                                        Operation::kRead))
        {
        case WaitResult::kTimedOut:
            return total_read;
        case WaitResult::kAborted:
            return failMsg<int>(error_callback, statusValue(StatusCode::Io::kAbortReadError), "Read aborted");
        case WaitResult::kError:
            return failErrno<int>(error_callback, statusValue(StatusCode::Io::kReadError));
        case WaitResult::kReady:
            break;
        }

        const int chunk_size = read_single_bytes ? 1 : (buffer_size - total_read);
        const ssize_t bytes_read =
            ::read(handle_context.file_descriptor, output + total_read, static_cast<std::size_t>(chunk_size));
        if (bytes_read < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }

            return failErrno<int>(error_callback, statusValue(StatusCode::Io::kReadError));
        }

        if (bytes_read == 0)
        {
            return total_read;
        }

        noteBytesRead(handle_context.state, static_cast<int>(bytes_read));
        total_read += static_cast<int>(bytes_read);

        if (matchesSuffix(output, total_read, terminator, terminator_size))
        {
            return total_read;
        }
    }

    return total_read;
}

inline auto writeImpl(int64_t handle, const void *buffer, int buffer_size, int timeout_ms, int multiplier,
                      ErrorCallbackT error_callback) -> int
{
    const auto callback = effectiveErrorCallback(error_callback);
    const auto buffer_status = cpp_core::validateBuffer<int>(buffer, buffer_size, callback);
    if (buffer_status < 0)
    {
        return buffer_status;
    }

    HandleContext handle_context;
    const auto handle_status = acquireHandleContext<int>(handle, callback, &handle_context);
    if (handle_status < 0)
    {
        return handle_status;
    }

    const auto *input = static_cast<const unsigned char *>(buffer);
    int total_written = 0;

    while (total_written < buffer_size)
    {
        const int current_timeout_ms =
            total_written == 0 ? cpp_core::clampTimeout(timeout_ms) : multiplierTimeout(timeout_ms, multiplier);
        switch (waitFileDescriptorReady(handle_context.state, handle_context.file_descriptor, current_timeout_ms,
                                        Operation::kWrite))
        {
        case WaitResult::kTimedOut:
            return total_written;
        case WaitResult::kAborted:
            return failMsg<int>(error_callback, statusValue(StatusCode::Io::kAbortWriteError), "Write aborted");
        case WaitResult::kError:
            return failErrno<int>(error_callback, statusValue(StatusCode::Io::kWriteError));
        case WaitResult::kReady:
            break;
        }

        const ssize_t bytes_written = ::write(handle_context.file_descriptor, input + total_written,
                                              static_cast<std::size_t>(buffer_size - total_written));
        if (bytes_written < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }

            return failErrno<int>(error_callback, statusValue(StatusCode::Io::kWriteError));
        }

        if (bytes_written == 0)
        {
            return total_written;
        }

        noteBytesWritten(handle_context.state, static_cast<int>(bytes_written));
        total_written += static_cast<int>(bytes_written);
    }

    return total_written;
}

} // namespace cpp_bindings_linux::detail
