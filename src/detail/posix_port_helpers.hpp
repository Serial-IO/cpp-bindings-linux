#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

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

inline auto readTrimmedFile(const std::filesystem::path &path) -> std::optional<std::string>
{
    std::ifstream stream(path);
    if (!stream.is_open())
    {
        return std::nullopt;
    }

    std::string contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    contents = trimWhitespace(std::move(contents));
    if (contents.empty())
    {
        return std::nullopt;
    }

    return contents;
}

inline auto isSerialDeviceName(std::string_view name) -> bool
{
    static constexpr std::string_view kPrefixes[] = {"ttyUSB", "ttyACM", "ttyS", "ttyAMA", "rfcomm", "ttyTHS"};
    return std::ranges::any_of(kPrefixes, [name](std::string_view prefix) { return name.starts_with(prefix); });
}

} // namespace cpp_bindings_linux::detail
