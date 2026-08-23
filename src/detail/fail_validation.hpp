#pragma once

#include "default_validation_message.hpp"
#include "fail_msg.hpp"

namespace cpp_bindings_linux::detail
{
template <typename ReturnType> inline auto failValidation(ErrorCallbackT error_callback, StatusCodeValue code) -> ReturnType
{
    return failMsg<ReturnType>(error_callback, code, defaultValidationMessage(code));
}
} // namespace cpp_bindings_linux::detail
