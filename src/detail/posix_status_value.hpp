#pragma once

#include "posix_common_types.hpp"

namespace cpp_bindings_linux::detail
{
template <typename Code> constexpr auto statusValue(Code code) -> StatusCodeValue
{
    return static_cast<StatusCodeValue>(code);
}
} // namespace cpp_bindings_linux::detail
