#pragma once

#include "posix_handle_types.hpp"
#include "posix_invoke_io_callback.hpp"

namespace cpp_bindings_linux::detail
{
inline auto noteBytesWritten(const std::shared_ptr<HandleState> &handle_state, int bytes_written) -> void
{
    handle_state->bytes_written_total.fetch_add(bytes_written, std::memory_order_relaxed);
    invokeIoCallback(g_write_callback.load(std::memory_order_acquire), bytes_written);
}
} // namespace cpp_bindings_linux::detail
