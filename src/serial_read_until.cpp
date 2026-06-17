#include <cpp_core/interface/serial_read_until.h>

#include "detail/posix_helpers.hpp"

extern "C"
{

    MODULE_API auto serialReadUntil(int64_t handle, void *buffer, int buffer_size, int timeout_ms, int multiplier,
                                    void *until_char, ErrorCallbackT error_callback) -> int
    {
        if (until_char == nullptr)
        {
            return cpp_bindings_linux::detail::failMsg<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Io::kBufferError),
                "Terminator pointer must not be null");
        }

        return cpp_bindings_linux::detail::readImpl(handle, buffer, buffer_size, timeout_ms, multiplier,
                                                    static_cast<const unsigned char *>(until_char), 1, error_callback);
    }

} // extern "C"
