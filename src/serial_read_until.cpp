#include "cpp_core/error_callback.h"
#include "cpp_core/status_codes.h"
#include "serial_internal.hpp"

#include <cpp_core/interface/serial_read.h>
#include <cpp_core/interface/serial_read_until.h>
#include <cstdint>
#include <utility>

using namespace serial_internal;

extern "C" auto serialReadUntil(int64_t handle_ptr, void *buffer, int buffer_size, int timeout_ms, int multiplier,
                                void *until_char_ptr, ErrorCallbackT error_callback) -> int
{
    if (buffer == nullptr || buffer_size <= 0 || until_char_ptr == nullptr)
    {
        return std::to_underlying(cpp_core::StatusCodes::kBufferError);
    }

    auto *handle = reinterpret_cast<SerialPortHandle *>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError),
            "serialReadUntil: Invalid handle",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    char const terminator = *static_cast<char *>(until_char_ptr);
    char *char_buf   = static_cast<char *>(buffer);
    int   total      = 0;

    while (total < buffer_size)
    {
        int const bytes = serialRead(handle_ptr, char_buf + total, 1, timeout_ms, multiplier, error_callback);
        if (bytes <= 0)
        {
            // timeout or error – propagate (if error negative, return negative)
            return (bytes == 0) ? total : bytes;
        }

        if (char_buf[total] == terminator)
        {
            ++total;
            break;
        }
        ++total;
    }

    return total;
}
