#include <cpp_core/interface/serial_close.h>
#include <cpp_core/status_codes.hpp>
#include <cpp_core/validation.hpp>

#include "detail/posix_helpers.hpp"

#include <unistd.h>

extern "C"
{

    MODULE_API auto serialClose(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        if (handle <= 0)
        {
            return 0;
        }

        const auto handle_ok = cpp_core::validateHandle<int>(handle, error_callback);
        if (handle_ok < 0)
        {
            return handle_ok;
        }

        const int fd = static_cast<int>(handle);
        if (close(fd) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kCloseHandleError);
        }

        return 0;
    }

} // extern "C"
