#pragma once

#include "posix_fail_msg.hpp"

#include <cerrno>
#include <system_error>

namespace cpp_bindings_linux::detail
{
template <typename ReturnType> inline auto failErrno(ErrorCallbackT error_callback, StatusCodeValue code) -> ReturnType
{
    return failMsg<ReturnType>(error_callback, code, std::error_code(errno, std::generic_category()).message());
}
} // namespace cpp_bindings_linux::detail
