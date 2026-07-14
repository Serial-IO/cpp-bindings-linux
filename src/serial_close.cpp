#include <cpp_core/interface/serial_close.h>

#include "detail/posix_fail_errno.hpp"
#include "detail/posix_fail_msg.hpp"
#include "detail/posix_remove_handle_state.hpp"
#include "detail/posix_status_value.hpp"

#include <limits>
#include <unistd.h>

extern "C"
{

    MODULE_API auto serialClose(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        if (handle <= 0)
        {
            return static_cast<int>(cpp_core::StatusCode::kSuccess);
        }

        if (handle > std::numeric_limits<int>::max())
        {
            return cpp_bindings_linux::detail::failMsg<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Connection::kInvalidHandleError),
                "Invalid handle");
        }

        const int file_descriptor = static_cast<int>(handle);
        if (close(file_descriptor) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Connection::kCloseHandleError));
        }

        cpp_bindings_linux::detail::removeHandleState(file_descriptor);
        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
