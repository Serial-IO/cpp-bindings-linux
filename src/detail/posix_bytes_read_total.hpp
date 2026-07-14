#pragma once

#include "posix_handle_types.hpp"

namespace cpp_bindings_linux::detail
{
inline auto bytesReadTotal(const std::shared_ptr<HandleState> &handle_state) -> int64_t
{
    return handle_state->bytes_read_total.load(std::memory_order_relaxed);
}
} // namespace cpp_bindings_linux::detail
