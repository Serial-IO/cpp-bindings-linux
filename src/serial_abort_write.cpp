#include "serial_internal.hpp"

#include <cpp_core/interface/serial_abort_write.h>

using namespace serial_internal;

extern "C" int serialAbortWrite(
    int64_t        handle_ptr,
    ErrorCallbackT error_callback
)
{
    auto *handle = reinterpret_cast<SerialPortHandle *>(handle_ptr);
    if (handle == nullptr)
    {
        invokeError(
            std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError),
            "serialAbortWrite: Invalid handle",
            error_callback
        );
        return std::to_underlying(cpp_core::StatusCodes::kInvalidHandleError);
    }

    handle->abort_write = true;
    return 0;
}
