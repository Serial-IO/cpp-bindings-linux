#pragma once

#include "posix_ensure_handle_state.hpp"
#include "posix_validate_file_descriptor.hpp"

namespace cpp_bindings_linux::detail
{
template <typename ReturnType>
inline auto acquireHandleContext(int64_t handle, ErrorCallbackT error_callback, HandleContext *out_handle_context)
    -> ReturnType
{
    int file_descriptor = -1;
    const auto validation_status = validatePosixFileDescriptor<ReturnType>(handle, error_callback, &file_descriptor);
    if (validation_status < 0)
    {
        return validation_status;
    }

    out_handle_context->file_descriptor = file_descriptor;
    out_handle_context->state = ensureHandleState(file_descriptor);
    return static_cast<ReturnType>(StatusCode::kSuccess);
}
} // namespace cpp_bindings_linux::detail
