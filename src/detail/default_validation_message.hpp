#pragma once

#include "common_types.hpp"

#include <string_view>

namespace cpp_bindings_linux::detail
{
inline auto defaultValidationMessage(StatusCodeValue code) -> std::string_view
{
    switch (code)
    {
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetBaudrateError):
        return "Invalid baudrate: must be >= 300";
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetDataBitsError):
        return "Invalid data bits: must be 5-8";
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetParityError):
        return "Invalid parity: must be 0, 1, or 2";
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetStopBitsError):
        return "Invalid stop bits: must be 0, 1, or 2";
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetFlowControlError):
        return "Invalid flow control mode: must be 0, 1, or 2";
    default:
        return "Invalid serial setting";
    }
}
} // namespace cpp_bindings_linux::detail
