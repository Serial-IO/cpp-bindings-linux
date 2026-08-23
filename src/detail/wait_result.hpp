#pragma once

namespace cpp_bindings_linux::detail
{
enum class WaitResult
{
    kReady,
    kTimedOut,
    kAborted,
    kError,
};
} // namespace cpp_bindings_linux::detail
