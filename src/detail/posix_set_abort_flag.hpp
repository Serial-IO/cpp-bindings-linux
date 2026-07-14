#pragma once

#include "posix_handle_types.hpp"

namespace cpp_bindings_linux::detail
{
inline auto setAbortFlag(const std::shared_ptr<HandleState> &handle_state, Operation operation) -> void
{
    if (operation == Operation::kRead)
    {
        handle_state->abort_read.store(true, std::memory_order_release);
    }
    else
    {
        handle_state->abort_write.store(true, std::memory_order_release);
    }
}
} // namespace cpp_bindings_linux::detail
