#pragma once

#include "posix_handle_types.hpp"

namespace cpp_bindings_linux::detail
{
inline auto ensureHandleState(int file_descriptor) -> std::shared_ptr<HandleState>
{
    std::lock_guard lock(g_handle_states_mutex);
    auto &handle_state = g_handle_states[file_descriptor];
    if (!handle_state)
    {
        handle_state = std::make_shared<HandleState>();
    }
    return handle_state;
}
} // namespace cpp_bindings_linux::detail
