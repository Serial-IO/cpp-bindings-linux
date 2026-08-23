#pragma once

#include "trim_whitespace.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <utility>

namespace cpp_bindings_linux::detail
{
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
} // namespace cpp_bindings_linux::detail
