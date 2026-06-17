#include <cpp_core/interface/serial_in_bytes_total.h>

#include "detail/posix_helpers.hpp"

extern "C"
{

    MODULE_API auto serialInBytesTotal(int64_t handle, ErrorCallbackT error_callback) -> int64_t
    {
        cpp_bindings_linux::detail::HandleContext context;
        const auto rc = cpp_bindings_linux::detail::acquireHandleContext<int64_t>(handle, error_callback, &context);
        if (rc < 0)
        {
            return rc;
        }

        return cpp_bindings_linux::detail::bytesReadTotal(context.state);
    }

} // extern "C"
