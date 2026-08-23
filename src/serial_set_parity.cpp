#include <cpp_core/interface/serial_set_parity.h>

#include "detail/acquire_handle_context.hpp"
#include "detail/apply_parity.hpp"
#include "detail/parse_parity.hpp"
#include "detail/read_termios2.hpp"
#include "detail/status_value.hpp"
#include "detail/write_termios2.hpp"
#include "detail/termios2.hpp"

#include <sys/ioctl.h>
#include <termios.h>

extern "C"
{

    MODULE_API auto serialSetParity(int64_t handle, int parity, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext handle_context;
        const auto status =
            cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &handle_context);
        if (status < 0)
        {
            return status;
        }

        const auto parity_value = cpp_bindings_linux::detail::parseParity(
            parity, error_callback,
            cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetParityError));
        if (!parity_value.has_value())
        {
            return static_cast<int>(cpp_core::StatusCode::Configuration::kSetParityError);
        }

        termios2 serial_settings{};
        if (cpp_bindings_linux::detail::readTermios2<int>(
                handle_context.file_descriptor, &serial_settings, error_callback) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Control::kGetStateError);
        }

        cpp_bindings_linux::detail::applyParity(&serial_settings, *parity_value);

        if (cpp_bindings_linux::detail::writeTermios2<int>(
                handle_context.file_descriptor, &serial_settings, error_callback,
                cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetParityError)) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Configuration::kSetParityError);
        }

        tcflush(handle_context.file_descriptor, TCIOFLUSH);

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
