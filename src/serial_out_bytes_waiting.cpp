#include "serial_internal.hpp"

#include <cpp_core/interface/serial_out_bytes_waiting.h>

using namespace serial_internal;

extern "C" int serialOutBytesWaiting(int64_t handle_ptr, ErrorCallbackT error_callback)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError), "serialOutBytesWaiting: Invalid handle", error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    std::lock_guard<std::recursive_mutex> guard(handle->mtx);

    int queued = 0;
#ifdef TIOCOUTQ
    if (ioctl(handle->fd, TIOCOUTQ, &queued) == -1)
    {
        invokeError(std::to_underlying(cpp_core::StatusCodes::kGetStateError), "serialOutBytesWaiting: ioctl failed", error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kGetStateError);
    }
#else
    queued = 0;
#endif
    return queued;
}
