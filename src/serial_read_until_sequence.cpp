#include <cpp_core/interface/serial_read_until_sequence.h>

#include "detail/fail_msg.hpp"
#include "detail/read_impl.hpp"
#include "detail/status_value.hpp"

#include <cstring>

extern "C"
{

    MODULE_API auto serialReadUntilSequence(int64_t handle, void *buffer, int buffer_size, int timeout_ms,
                                            int multiplier, void *sequence, ErrorCallbackT error_callback) -> int
    {
        if (sequence == nullptr)
        {
            return cpp_bindings_linux::detail::failMsg<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Io::kBufferError),
                "Sequence pointer must not be null");
        }

        const auto *sequence_bytes = static_cast<const unsigned char *>(sequence);
        const int sequence_size = static_cast<int>(std::strlen(reinterpret_cast<const char *>(sequence_bytes)));
        if (sequence_size <= 0)
        {
            return cpp_bindings_linux::detail::failMsg<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Io::kBufferError),
                "Sequence must not be empty");
        }

        return cpp_bindings_linux::detail::readImpl(handle, buffer, buffer_size, timeout_ms, multiplier, sequence_bytes,
                                                    sequence_size, error_callback);
    }

} // extern "C"
