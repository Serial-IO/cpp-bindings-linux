#pragma once

#include "posix_fail_validation.hpp"

#include <optional>

namespace cpp_bindings_linux::detail
{
inline auto parseStopBits(int stop_bits, ErrorCallbackT error_callback, StatusCodeValue invalid_code)
    -> std::optional<StopBits>
{
    switch (stop_bits)
    {
    case 0:
    case 1:
        return StopBits::kOne;
    case 2:
        return StopBits::kTwo;
    default:
        break;
    }

    (void)failValidation<int>(error_callback, invalid_code);
    return std::nullopt;
}
} // namespace cpp_bindings_linux::detail
