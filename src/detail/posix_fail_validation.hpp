#pragma once

#include "posix_default_validation_message.hpp"
#include "posix_fail_msg.hpp"

namespace cpp_bindings_linux::detail
{
template <typename Ret> inline auto failValidation(ErrorCallbackT error_callback, StatusCodeValue code) -> Ret
{
    return failMsg<Ret>(error_callback, code, defaultValidationMessage(code));
}
} // namespace cpp_bindings_linux::detail
