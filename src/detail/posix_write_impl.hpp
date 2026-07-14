#pragma once

#include "posix_acquire_handle_context.hpp"
#include "posix_effective_error_callback.hpp"
#include "posix_fail_errno.hpp"
#include "posix_fail_msg.hpp"
#include "posix_multiplier_timeout.hpp"
#include "posix_note_bytes_written.hpp"
#include "posix_status_value.hpp"
#include "posix_wait_file_descriptor_ready.hpp"

#include <cpp_core/validation.hpp>

#include <cerrno>
#include <unistd.h>

namespace cpp_bindings_linux::detail
{
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
