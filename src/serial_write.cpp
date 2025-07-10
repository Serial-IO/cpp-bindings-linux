#include "serial_internal.hpp"

#include <cpp_core/interface/serial_write.h>

using namespace serial_internal;

extern "C" int
serialWrite(int64_t handle_ptr, const void* buffer, int buffer_size, int timeout_ms, int /*multiplier*/, ErrorCallbackT error_callback)
{
    if (buffer == nullptr || buffer_size <= 0)
    {
        return std::to_underlying(cpp_core::StatusCodes::kBufferError);
    }

    auto* handle = reinterpret_cast<SerialPortHandle*>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError), "serialWrite: Invalid handle", error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    // Abort?
    if (handle->abort_write.exchange(false))
    {
        return std::to_underlying(cpp_core::StatusCodes::kAbortWriteError);
    }

    if (waitFdReady(handle->fd, timeout_ms, true) <= 0)
    {
        return 0; // timeout
    }

    ssize_t bytes_written = ::write(handle->fd, buffer, buffer_size);
    if (bytes_written < 0)
    {
        invokeError(std::to_underlying(cpp_core::StatusCodes::kWriteError), "serialWrite: Write error", error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kWriteError);
    }

    if (bytes_written > 0)
    {
        handle->tx_total += bytes_written;
        if (g_write_callback != nullptr)
        {
            g_write_callback(static_cast<int>(bytes_written));
        }
    }

    return static_cast<int>(bytes_written);
}
