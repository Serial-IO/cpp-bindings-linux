#pragma once

#include "acquire_handle_context.hpp"
#include "effective_error_callback.hpp"
#include "fail_errno.hpp"
#include "fail_msg.hpp"
#include "matches_suffix.hpp"
#include "multiplier_timeout.hpp"
#include "note_bytes_read.hpp"
#include "status_value.hpp"
#include "wait_file_descriptor_ready.hpp"

#include <cpp_core/validation.hpp>

#include <cerrno>
#include <unistd.h>

namespace cpp_bindings_linux::detail
{
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
} // namespace cpp_bindings_linux::detail
