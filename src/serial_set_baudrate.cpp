#include <cpp_core/interface/serial_set_baudrate.h>

#include "detail/posix_helpers.hpp"
#include "detail/posix_termios2.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialSetBaudrate(int64_t handle, int baudrate, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext context;
        const auto rc = cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &context);
        if (rc < 0)
        {
            return rc;
        }

        if (!cpp_bindings_linux::detail::validateBaudrateValue(baudrate))
        {
            return cpp_bindings_linux::detail::failValidation<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetBaudrateError));
        }

        termios2 tty{};
        if (cpp_bindings_linux::detail::readTermios2<int>(context.fd, &tty, error_callback) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Control::kGetStateError);
        }

        cpp_bindings_linux::detail::applyBaudrate(&tty, baudrate);

        if (cpp_bindings_linux::detail::writeTermios2<int>(
                context.fd, &tty, error_callback,
                cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetBaudrateError)) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Configuration::kSetBaudrateError);
        }

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
