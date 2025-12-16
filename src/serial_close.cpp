#include <cpp_core/interface/serial_close.h>
#include <cpp_core/status_codes.h>

#include "detail/posix_helpers.hpp"

#include <limits>
#include <unistd.h>

extern "C"
{

    MODULE_API auto serialClose(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        if (handle <= 0)
        {
            return static_cast<int>(cpp_core::StatusCodes::kSuccess);
        }
        if (handle > std::numeric_limits<int>::max())
        {
            return cpp_bindings_linux::detail::failMsg<int>(error_callback, cpp_core::StatusCodes::kInvalidHandleError,
                                                            "Invalid handle");
        }

        const int fd = static_cast<int>(handle);
        if (close(fd) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kCloseHandleError);
        }

        return static_cast<int>(cpp_core::StatusCodes::kSuccess);
    }

} // extern "C"
