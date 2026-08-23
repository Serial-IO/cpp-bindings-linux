#pragma once

#include "default_validation_message.hpp"
#include "status_value.hpp"

#include <cpp_core/error_handling.hpp>

#include <string>

namespace cpp_bindings_linux::detail
{
inline auto validateBaudrateValue(int baudrate) -> cpp_core::Status
{
    if (cpp_core::SerialConfig::tryMake(baudrate, 8))
    {
        return cpp_core::ok();
    }

    return cpp_core::fail<>(statusValue(StatusCode::Configuration::kSetBaudrateError),
                            std::string(defaultValidationMessage(
                                statusValue(StatusCode::Configuration::kSetBaudrateError))));
}
} // namespace cpp_bindings_linux::detail
