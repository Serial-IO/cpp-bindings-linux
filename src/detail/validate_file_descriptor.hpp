#pragma once

#include "effective_error_callback.hpp"

#include <cpp_core/validation.hpp>

#include <cstdint>

namespace cpp_bindings_linux::detail
{
template <typename ReturnType>
inline auto validatePosixFileDescriptor(int64_t handle, ErrorCallbackT error_callback, int *out_file_descriptor)
    -> ReturnType
{
    const auto validation_status = cpp_core::validateHandle<ReturnType>(handle, effectiveErrorCallback(error_callback));
    if (validation_status < 0)
    {
        return validation_status;
    }

    *out_file_descriptor = static_cast<int>(handle);
    return static_cast<ReturnType>(StatusCode::kSuccess);
}
} // namespace cpp_bindings_linux::detail
