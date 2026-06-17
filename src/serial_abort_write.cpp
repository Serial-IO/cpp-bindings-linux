#include <cpp_core/interface/serial_abort_write.h>

#include "detail/posix_helpers.hpp"

extern "C"
{

    MODULE_API auto serialAbortWrite(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext context;
        const auto rc = cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &context);
        if (rc < 0)
        {
            return rc;
        }

        cpp_bindings_linux::detail::setAbortFlag(context.state, cpp_bindings_linux::detail::Operation::kWrite);
        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
