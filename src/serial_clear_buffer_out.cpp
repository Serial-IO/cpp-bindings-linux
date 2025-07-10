#include "serial_internal.hpp"

#include <cpp_core/interface/serial_clear_buffer_out.h>

using namespace serial_internal;

extern "C" int serialClearBufferOut(int64_t handle_ptr, ErrorCallbackT error_callback)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError), "serialClearBufferOut: Invalid handle", error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    if (tcflush(handle->fd, TCOFLUSH) != 0)
    {
        invokeError(std::to_underlying(cpp_core::StatusCodes::kClearBufferOutError),
                    "serialClearBufferOut: Failed to flush output buffer",
                    error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kClearBufferOutError);
    }
    return 0;
}
