#pragma once

#include <algorithm>
#include <string_view>

namespace cpp_bindings_linux::detail
{
inline auto isSerialDeviceName(std::string_view name) -> bool
{
    static constexpr std::string_view kPrefixes[] = {"ttyUSB", "ttyACM", "ttyS", "ttyAMA", "rfcomm", "ttyTHS"};
    return std::ranges::any_of(kPrefixes, [name](std::string_view prefix) { return name.starts_with(prefix); });
}
} // namespace cpp_bindings_linux::detail
