#include "cpp_core/error_callback.h"
#include "serial_internal.hpp"

#include <cpp_core/interface/serial_out_bytes_total.h>
#include <cstdint>
#include <mutex>

using namespace serial_internal;

extern "C" auto serialOutBytesTotal(int64_t handle_ptr, ErrorCallbackT /*error_callback*/
                                    ) -> int64_t
{
    auto *handle = reinterpret_cast<SerialPortHandle *>(handle_ptr);
    if (handle == nullptr)
    {
        return 0;
    }

    std::scoped_lock const guard(handle->mtx);
    return handle->tx_total;
}
