#pragma once

#include "posix_default_validation_message.hpp"
#include "posix_status_value.hpp"

#include <cpp_core/error_handling.hpp>

#include <string>

namespace cpp_bindings_linux::detail
{
inline auto validateDataBitsValue(int data_bits) -> cpp_core::Status
{
    if (cpp_core::SerialConfig::tryMake(300, data_bits))
    {
        return cpp_core::ok();
    }

    return cpp_core::fail<>(statusValue(StatusCode::Configuration::kSetDataBitsError),
                            std::string(defaultValidationMessage(
                                statusValue(StatusCode::Configuration::kSetDataBitsError))));
}
} // namespace cpp_bindings_linux::detail
