#include "serial_internal.hpp"

#include <cpp_core/interface/serial_clear_buffer_in.h>

using namespace serial_internal;

extern "C" int serialClearBufferIn(int64_t handle_ptr, ErrorCallbackT error_callback)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError), "serialClearBufferIn: Invalid handle", error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    std::lock_guard<std::recursive_mutex> guard(handle->mtx);

    if (tcflush(handle->fd, TCIFLUSH) != 0)
    {
        invokeError(std::to_underlying(cpp_core::StatusCodes::kClearBufferInError),
                    "serialClearBufferIn: Failed to flush input buffer",
                    error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kClearBufferInError);
    }
    return 0;
}
