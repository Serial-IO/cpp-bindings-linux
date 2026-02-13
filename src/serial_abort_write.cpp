#include <cpp_core/interface/serial_abort_write.h>
#include <cpp_core/status_codes.h>

#include "detail/abort_registry.hpp"
#include "detail/posix_helpers.hpp"

#include <cerrno>
#include <limits>
#include <unistd.h>

extern "C"
{
    MODULE_API auto serialAbortWrite(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        if (handle <= 0 || handle > std::numeric_limits<int>::max())
        {
            return cpp_bindings_linux::detail::failMsg<int>(error_callback, cpp_core::StatusCodes::kInvalidHandleError,
                                                            "Invalid handle");
        }

        const int fd = static_cast<int>(handle);
        const auto pipes = cpp_bindings_linux::detail::getAbortPipesForFd(fd);
        if (!pipes)
        {
            return cpp_bindings_linux::detail::failMsg<int>(error_callback, cpp_core::StatusCodes::kInvalidHandleError,
                                                            "Invalid handle");
        }

        const unsigned char token = 1;
        for (;;)
        {
            const ssize_t n = ::write(pipes->write_abort_w, &token, 1);
            if (n == 1)
            {
                return static_cast<int>(cpp_core::StatusCodes::kSuccess);
            }
            if (n < 0 && errno == EINTR)
            {
                continue;
            }
            // Pipe already full -> treat as success (abort already requested).
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                return static_cast<int>(cpp_core::StatusCodes::kSuccess);
            }
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kAbortWriteError);
        }
    }
} // extern "C"
