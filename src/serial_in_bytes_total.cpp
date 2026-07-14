#include <cpp_core/interface/serial_in_bytes_total.h>

#include "detail/posix_acquire_handle_context.hpp"
#include "detail/posix_bytes_read_total.hpp"

extern "C"
{

    MODULE_API auto serialInBytesTotal(int64_t handle, ErrorCallbackT error_callback) -> int64_t
    {
        cpp_bindings_linux::detail::HandleContext handle_context;
        const auto status =
            cpp_bindings_linux::detail::acquireHandleContext<int64_t>(handle, error_callback, &handle_context);
        if (status < 0)
        {
            return status;
        }

        return cpp_bindings_linux::detail::bytesReadTotal(handle_context.state);
    }

} // extern "C"
