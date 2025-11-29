#include "cpp_core/error_callback.h"
#include "cpp_core/status_codes.h"
#include "serial_internal.hpp"

#include <cpp_core/interface/serial_read.h>
#include <cpp_core/interface/serial_read_until_sequence.h>
#include <cstdint>
#include <cstring>
#include <utility>

using namespace serial_internal;

extern "C" auto serialReadUntilSequence(int64_t handle_ptr, void *buffer, int buffer_size, int timeout_ms,
                                        int multiplier, void *sequence_ptr, ErrorCallbackT error_callback) -> int
{
    if (buffer == nullptr || buffer_size <= 0 || sequence_ptr == nullptr)
    {
        return std::to_underlying(cpp_core::StatusCodes::kBufferError);
    }

    auto *handle = reinterpret_cast<SerialPortHandle *>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError),
            "serialReadUntilSequence: Invalid handle",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    const char *sequence_cstr = static_cast<const char *>(sequence_ptr);
    const int   seq_len       = static_cast<int>(std::strlen(sequence_cstr));
    if (seq_len == 0 || buffer_size < seq_len)
    {
        return 0;
    }

    char *char_buf = static_cast<char *>(buffer);
    int   total    = 0;
    int   matched  = 0;

    while (total < buffer_size)
    {
        int const bytes = serialRead(handle_ptr, char_buf + total, 1, timeout_ms, multiplier, error_callback);
        if (bytes <= 0)
        {
            return (bytes == 0) ? total : bytes;
        }

        char const current = char_buf[total];
        ++total;

        if (current == sequence_cstr[matched])
        {
            ++matched;
            if (matched == seq_len)
            {
                break; // fully matched
            }
        }
        else
        {
            matched = (current == sequence_cstr[0]) ? 1 : 0;
        }
    }

    return total;
}
