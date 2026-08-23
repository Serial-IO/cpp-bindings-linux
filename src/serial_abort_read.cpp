#include <cpp_core/interface/serial_abort_read.h>

#include "detail/acquire_handle_context.hpp"
#include "detail/set_abort_flag.hpp"

extern "C"
{

    MODULE_API auto serialAbortRead(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext handle_context;
        const auto status =
            cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &handle_context);
        if (status < 0)
        {
            return status;
        }

        cpp_bindings_linux::detail::setAbortFlag(handle_context.state, cpp_bindings_linux::detail::Operation::kRead);
        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
