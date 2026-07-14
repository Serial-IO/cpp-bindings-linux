#pragma once

#include "posix_fail_validation.hpp"

#include <optional>

namespace cpp_bindings_linux::detail
{
inline auto parseFlowControl(int mode, ErrorCallbackT error_callback, StatusCodeValue invalid_code)
    -> std::optional<FlowControl>
{
    switch (mode)
    {
    case 0:
        return FlowControl::kNone;
    case 1:
        return FlowControl::kRtsCts;
    case 2:
        return FlowControl::kXonXoff;
    default:
        (void)failValidation<int>(error_callback, invalid_code);
        return std::nullopt;
    }
}
} // namespace cpp_bindings_linux::detail
