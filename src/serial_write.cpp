#include "cpp_core/error_callback.h"
#include "cpp_core/status_codes.h"
#include "serial_internal.hpp"

#include <cpp_core/interface/serial_write.h>
#include <cstdint>
#include <mutex>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

using namespace serial_internal;

extern "C" auto serialWrite(int64_t handle_ptr, const void *buffer, int buffer_size, int timeout_ms, int /*multiplier*/,
                            ErrorCallbackT error_callback) -> int
{
    if (buffer == nullptr || buffer_size <= 0)
    {
        return std::to_underlying(cpp_core::StatusCodes::kBufferError);
    }

    auto *handle = reinterpret_cast<SerialPortHandle *>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError),
            "serialWrite: Invalid handle",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    std::scoped_lock const guard(handle->mtx);

    // Abort?
    if (handle->abort_write.exchange(false))
    {
        return std::to_underlying(cpp_core::StatusCodes::kAbortWriteError);
    }

    if (waitFdReady(handle->fd, timeout_ms, true) <= 0)
    {
        return 0; // timeout
    }

    ssize_t const bytes_written = ::write(handle->fd, buffer, buffer_size);
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
