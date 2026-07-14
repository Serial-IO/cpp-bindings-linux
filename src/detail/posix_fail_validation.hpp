#pragma once

#include "posix_default_validation_message.hpp"
#include "posix_fail_msg.hpp"

namespace cpp_bindings_linux::detail
{
template <typename ReturnType> inline auto failValidation(ErrorCallbackT error_callback, StatusCodeValue code) -> ReturnType
{
    return failMsg<ReturnType>(error_callback, code, defaultValidationMessage(code));
}
} // namespace cpp_bindings_linux::detail
