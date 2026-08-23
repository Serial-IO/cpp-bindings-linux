#pragma once

#include "handle_types.hpp"

namespace cpp_bindings_linux::detail
{
inline auto consumeAbortFlag(const std::shared_ptr<HandleState> &handle_state, Operation operation) -> bool
{
    if (operation == Operation::kRead)
    {
        return handle_state->abort_read.exchange(false, std::memory_order_acq_rel);
    }

    return handle_state->abort_write.exchange(false, std::memory_order_acq_rel);
}
} // namespace cpp_bindings_linux::detail
