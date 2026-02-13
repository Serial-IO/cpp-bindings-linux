#pragma once

#include <optional>

namespace cpp_bindings_linux::detail
{
struct AbortPipes
{
    int read_abort_r = -1;
    int read_abort_w = -1;
    int write_abort_r = -1;
    int write_abort_w = -1;
};

// Create (if needed) and register abort pipes for a serial FD.
// Returns true on success.
auto registerAbortPipesForFd(int fd) -> bool;

// Unregister and close any abort pipes for a serial FD. Safe to call multiple times.
auto unregisterAbortPipesForFd(int fd) -> void;

// Returns the abort pipes for a serial FD, or std::nullopt if none exist.
auto getAbortPipesForFd(int fd) -> std::optional<AbortPipes>;
} // namespace cpp_bindings_linux::detail
