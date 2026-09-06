#include <cpp_core/interface/serial_get_data_bits.h>

#include "detail/acquire_handle_context.hpp"
#include "detail/fail_errno.hpp"
#include "detail/status_value.hpp"
#include "detail/termios2.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialGetDataBits(int64_t handle, ErrorCallbackT error_callback) -> cpp_core::DataBits
    {
        cpp_bindings_linux::detail::HandleContext handle_context;
        const auto status =
            cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &handle_context);
        if (status < 0)
        {
            return static_cast<cpp_core::DataBits>(status);
        }

        termios2 serial_settings{};
        if (ioctl(handle_context.file_descriptor, TCGETS2, &serial_settings) != 0)
        {
            return static_cast<cpp_core::DataBits>(cpp_bindings_linux::detail::failErrno<int>(
                error_callback,
                cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kGetStateError)));
        }

        switch (serial_settings.c_cflag & CSIZE)
        {
        case CS5:
            return cpp_core::DataBits::kFive;
        case CS6:
            return cpp_core::DataBits::kSix;
        case CS7:
            return cpp_core::DataBits::kSeven;
        case CS8:
        default:
            return cpp_core::DataBits::kEight;
        }
    }

} // extern "C"
