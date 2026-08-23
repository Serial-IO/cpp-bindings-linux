#pragma once

#include <cstring>

namespace cpp_bindings_linux::detail
{
inline auto matchesSuffix(const unsigned char *buffer, int buffer_size, const unsigned char *terminator,
                          int terminator_size) -> bool
{
    if (terminator == nullptr || terminator_size <= 0 || buffer_size < terminator_size)
    {
        return false;
    }

    return std::memcmp(buffer + (buffer_size - terminator_size), terminator,
                       static_cast<std::size_t>(terminator_size)) == 0;
}
} // namespace cpp_bindings_linux::detail
