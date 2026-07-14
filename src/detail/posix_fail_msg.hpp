#pragma once

#include "posix_effective_error_callback.hpp"

#include <cpp_core/error_handling.hpp>

#include <string_view>

namespace cpp_bindings_linux::detail
{
template <typename ReturnType>
inline auto failMsg(ErrorCallbackT error_callback, StatusCodeValue code, std::string_view message) -> ReturnType
{
    return cpp_core::failMsg<ReturnType>(effectiveErrorCallback(error_callback), code, message);
}
} // namespace cpp_bindings_linux::detail
