#pragma once

#include <cctype>
#include <string>

namespace cpp_bindings_linux::detail
{
inline auto trimWhitespace(std::string value) -> std::string
{
    const auto is_space = [](unsigned char character) { return std::isspace(character) != 0; };
    while (!value.empty() && is_space(static_cast<unsigned char>(value.front())))
    {
        value.erase(value.begin());
    }
    while (!value.empty() && is_space(static_cast<unsigned char>(value.back())))
    {
        value.pop_back();
    }

    return value;
}
} // namespace cpp_bindings_linux::detail
