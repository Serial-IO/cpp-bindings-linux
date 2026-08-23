#pragma once

#include <cpp_core/validation.hpp>

namespace cpp_bindings_linux::detail
{
inline auto multiplierTimeout(int timeout_ms, int multiplier) -> int
{
    if (multiplier <= 0)
    {
        return 0;
    }

    return cpp_core::clampTimeout(timeout_ms) * multiplier;
}
} // namespace cpp_bindings_linux::detail
