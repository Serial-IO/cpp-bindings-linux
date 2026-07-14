#pragma once

#include "posix_handle_types.hpp"
#include "posix_invoke_io_callback.hpp"

namespace cpp_bindings_linux::detail
{
inline auto noteBytesRead(const std::shared_ptr<HandleState> &handle_state, int bytes_read) -> void
{
    handle_state->bytes_read_total.fetch_add(bytes_read, std::memory_order_relaxed);
    invokeIoCallback(g_read_callback.load(std::memory_order_acquire), bytes_read);
}
} // namespace cpp_bindings_linux::detail
