#include "cpp_core/error_callback.h"
#include "cpp_core/status_codes.h"
#include "serial_internal.hpp"

#include <cpp_core/interface/serial_abort_write.h>
#include <cstdint>
#include <utility>

using namespace serial_internal;

extern "C" auto serialAbortWrite(int64_t handle_ptr, ErrorCallbackT error_callback) -> int
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
