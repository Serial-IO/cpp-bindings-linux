#include "serial_internal.hpp"

#include <cpp_core/interface/serial_out_bytes_total.h>

using namespace serial_internal;

extern "C" int64_t serialOutBytesTotal(int64_t handle_ptr, ErrorCallbackT /*error_callback*/)
{
    auto* handle = reinterpret_cast<SerialPortHandle*>(handle_ptr);
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->tx_total;
}
