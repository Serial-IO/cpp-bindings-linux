#include "serial_internal.hpp"

#include <cpp_core/interface/serial_in_bytes_waiting.h>

using namespace serial_internal;

extern "C" int serialInBytesWaiting(int64_t handle_ptr, ErrorCallbackT error_callback)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError), "serialInBytesWaiting: Invalid handle", error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    int available = 0;
    if (ioctl(handle->fd, FIONREAD, &available) == -1)
    {
        invokeError(std::to_underlying(cpp_core::StatusCodes::kGetStateError), "serialInBytesWaiting: ioctl failed", error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kGetStateError);
    }
    return available;
}
