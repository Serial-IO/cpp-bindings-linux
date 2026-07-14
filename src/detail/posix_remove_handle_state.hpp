#pragma once

#include "posix_handle_types.hpp"

namespace cpp_bindings_linux::detail
{
inline auto removeHandleState(int file_descriptor) -> void
{
    std::lock_guard lock(g_handle_states_mutex);
    g_handle_states.erase(file_descriptor);
}
} // namespace cpp_bindings_linux::detail
