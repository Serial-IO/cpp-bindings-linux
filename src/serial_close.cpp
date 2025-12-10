#include <cpp_core/interface/serial_close.h>
#include <cpp_core/status_codes.h>

#include <cerrno>
#include <system_error>
#include <unistd.h>

extern "C"
{

    MODULE_API auto serialClose(int64_t handle, ErrorCallbackT error_callback) -> int
    {
        if (handle <= 0)
        {
            return static_cast<int>(cpp_core::StatusCodes::kSuccess);
        }

        const int fd = static_cast<int>(handle);
        if (close(fd) != 0)
        {
            if (error_callback != nullptr)
            {
                const std::string error_msg = std::error_code(errno, std::generic_category()).message();
                error_callback(static_cast<int>(cpp_core::StatusCodes::kCloseHandleError), error_msg.c_str());
            }
            return static_cast<int>(cpp_core::StatusCodes::kCloseHandleError);
        }

        return static_cast<int>(cpp_core::StatusCodes::kSuccess);
    }

} // extern "C"
