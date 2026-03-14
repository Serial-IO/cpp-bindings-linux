#include <cpp_core/interface/serial_set_dtr.h>
#include <cpp_core/status_codes.h>

#include "detail/posix_helpers.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialSetDtr(int64_t handle, int state, ErrorCallbackT error_callback) -> int
    {
        int fd = -1;
        const auto rc = cpp_bindings_linux::detail::validatePosixFd<int>(handle, error_callback, &fd);
        if (rc < 0)
        {
            return rc;
        }

        int flag = TIOCM_DTR;
        if (ioctl(fd, state ? TIOCMBIS : TIOCMBIC, &flag) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback, cpp_core::StatusCodes::kSetDtrError);
        }

        return static_cast<int>(cpp_core::StatusCodes::kSuccess);
    }

} // extern "C"
