#include "serial_internal.hpp"

#include <cpp_core/interface/serial_read.h>

using namespace serial_internal;

extern "C" int serialRead(
    int64_t handle_ptr,
    void   *buffer,
    int     buffer_size,
    int     timeout_ms,
    int /*multiplier*/,
    ErrorCallbackT error_callback
)
{
    if (buffer == nullptr || buffer_size <= 0)
    {
        return std::to_underlying(cpp_core::StatusCodes::kBufferError);
    }

    auto *handle = reinterpret_cast<SerialPortHandle *>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError), "serialRead: Invalid handle", error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    std::lock_guard<std::recursive_mutex> guard(handle->mtx);

    // Abort requested from another thread?
    if (handle->abort_read.exchange(false))
    {
        return std::to_underlying(cpp_core::StatusCodes::kAbortReadError);
    }

    if (waitFdReady(handle->fd, timeout_ms, false) <= 0)
    {
        return 0; // timeout or error -> treat as timeout, returning 0 bytes
    }

    ssize_t bytes_read_system = ::read(handle->fd, buffer, buffer_size);
    if (bytes_read_system < 0)
    {
        invokeError(std::to_underlying(cpp_core::StatusCodes::kReadError), "serialRead: Read error", error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kReadError);
    }

    if (bytes_read_system > 0)
    {
        handle->rx_total += bytes_read_system;
        if (g_read_callback != nullptr)
        {
            g_read_callback(static_cast<int>(bytes_read_system));
        }
    }

    return static_cast<int>(bytes_read_system);
}
