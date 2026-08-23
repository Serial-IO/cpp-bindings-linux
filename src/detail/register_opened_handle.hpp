#pragma once

#include "ensure_handle_state.hpp"

namespace cpp_bindings_linux::detail
{
inline auto registerOpenedHandle(int file_descriptor) -> void
{
    (void)ensureHandleState(file_descriptor);
}
} // namespace cpp_bindings_linux::detail
