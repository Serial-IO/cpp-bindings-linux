#pragma once

#include "common_types.hpp"

namespace cpp_bindings_linux::detail
{
inline auto invokeIoCallback(IoCallbackT callback, int transferred_bytes) -> void
{
    if (callback != nullptr)
    {
        callback(transferred_bytes);
    }
}
} // namespace cpp_bindings_linux::detail
