#include "serial_internal.hpp"

#include <cpp_core/interface/serial_drain.h>

using namespace serial_internal;

extern "C" int serialDrain(int64_t handle_ptr, ErrorCallbackT error_callback)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError), "serialDrain: Invalid handle", error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    std::lock_guard<std::recursive_mutex> guard(handle->mtx);

    if (tcdrain(handle->fd) != 0)
    {
        invokeError(std::to_underlying(cpp_core::StatusCodes::kGetStateError), "serialDrain: tcdrain failed", error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kGetStateError);
    }
    return 0;
}
