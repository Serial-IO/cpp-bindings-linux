#pragma once

#include "fail_validation.hpp"

#include <optional>

namespace cpp_bindings_linux::detail
{
inline auto parseParity(int parity, ErrorCallbackT error_callback, StatusCodeValue invalid_code) -> std::optional<Parity>
{
    switch (parity)
    {
    case 0:
        return Parity::kNone;
    case 1:
        return Parity::kEven;
    case 2:
        return Parity::kOdd;
    default:
        (void)failValidation<int>(error_callback, invalid_code);
        return std::nullopt;
    }
}
} // namespace cpp_bindings_linux::detail
