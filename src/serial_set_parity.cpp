#include <cpp_core/interface/serial_set_parity.h>

#include "detail/posix_helpers.hpp"
#include "detail/posix_termios2.hpp"

#include <sys/ioctl.h>
#include <termios.h>

extern "C"
{

    MODULE_API auto serialSetParity(int64_t handle, int parity, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext context;
        const auto rc = cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &context);
        if (rc < 0)
        {
            return rc;
        }

        const auto parity_value = cpp_bindings_linux::detail::parseParity(
            parity, error_callback,
            cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetParityError));
        if (!parity_value.has_value())
        {
            return static_cast<int>(cpp_core::StatusCode::Configuration::kSetParityError);
        }

        termios2 tty{};
        if (cpp_bindings_linux::detail::readTermios2<int>(context.fd, &tty, error_callback) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Control::kGetStateError);
        }

        cpp_bindings_linux::detail::applyParity(&tty, *parity_value);

        if (cpp_bindings_linux::detail::writeTermios2<int>(
                context.fd, &tty, error_callback,
                cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetParityError)) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Configuration::kSetParityError);
        }

        tcflush(context.fd, TCIOFLUSH);

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
