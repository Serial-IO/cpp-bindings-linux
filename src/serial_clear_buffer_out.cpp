#include <cpp_core/interface/serial_clear_buffer_out.h>

#include "detail/posix_acquire_handle_context.hpp"
#include "detail/posix_fail_errno.hpp"
#include "detail/posix_status_value.hpp"

#include <termios.h>

extern "C"
{

    MODULE_API auto serialClearBufferOut(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext handle_context;
        const auto status =
            cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &handle_context);
        if (status < 0)
        {
            return status;
        }

        if (tcdrain(handle_context.file_descriptor) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Io::kClearBufferOutError));
        }

        if (tcflush(handle_context.file_descriptor, TCOFLUSH) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Io::kClearBufferOutError));
        }

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
