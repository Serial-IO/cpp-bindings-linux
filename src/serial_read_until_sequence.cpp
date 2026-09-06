#include <cpp_core/interface/serial_read_until_sequence.h>

#include "detail/fail_msg.hpp"
#include "detail/read_impl.hpp"
#include "detail/status_value.hpp"

extern "C"
{

    MODULE_API auto serialReadUntilSequence(int64_t handle, std::uint8_t *buffer, int buffer_size,
                                            const cpp_core::SerialTimeoutConfig *timeout_config,
                                            const std::uint8_t *sequence, int sequence_size,
                                            ErrorCallbackT error_callback) -> int
    {
        if (sequence == nullptr || sequence_size <= 0)
        {
            return cpp_bindings_linux::detail::failMsg<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Io::kBufferError),
                "Sequence must not be null and sequence_size must be positive");
        }

        return cpp_bindings_linux::detail::readImpl(handle, buffer, buffer_size, timeout_config, sequence,
                                                    sequence_size, error_callback);
    }

} // extern "C"
