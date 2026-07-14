#pragma once

#include "posix_common_types.hpp"

namespace cpp_bindings_linux::detail
{
inline auto effectiveErrorCallback(ErrorCallbackT error_callback) -> ErrorCallbackT
{
    return error_callback != nullptr ? error_callback : g_error_callback.load(std::memory_order_acquire);
}
} // namespace cpp_bindings_linux::detail
