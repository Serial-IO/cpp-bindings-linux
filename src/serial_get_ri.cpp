#include <cpp_core/interface/serial_get_ri.h>
#include <cpp_core/status_codes.h>

#include "detail/posix_helpers.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialGetRi(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        int fd = -1;
        const auto rc = cpp_bindings_linux::detail::validatePosixFd<int>(handle, error_callback, &fd);
        if (rc < 0)
        {
            return rc;
        }

        int status = 0;
        if (ioctl(fd, TIOCMGET, &status) != 0)
        {
            return cpp_bindings_linux::detail::failErrno<int>(error_callback,
                                                              cpp_core::StatusCodes::kGetModemStatusError);
        }

        return (status & TIOCM_RNG) ? 1 : 0;
    }

} // extern "C"
