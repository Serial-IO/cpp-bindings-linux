#include "serial_internal.hpp"

#include <cpp_core/interface/serial_abort_read.h>

using namespace serial_internal;

extern "C" int serialAbortRead(int64_t handle_ptr, ErrorCallbackT error_callback)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError), "serialAbortRead: Invalid handle", error_callback);
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    handle->abort_read = true;
    return 0;
}
