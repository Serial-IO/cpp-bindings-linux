#include <cpp_core/interface/serial_set_stop_bits.h>

#include "detail/posix_helpers.hpp"
#include "detail/posix_termios2.hpp"

#include <sys/ioctl.h>
#include <termios.h>

extern "C"
{

    MODULE_API auto serialSetStopBits(int64_t handle, int stop_bits, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext handle_context;
        const auto status =
            cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &handle_context);
        if (status < 0)
        {
            return status;
        }

        const auto stop_bits_value = cpp_bindings_linux::detail::parseStopBits(
            stop_bits, error_callback,
            cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetStopBitsError));
        if (!stop_bits_value.has_value())
        {
            return static_cast<int>(cpp_core::StatusCode::Configuration::kSetStopBitsError);
        }

        termios2 serial_settings{};
        if (cpp_bindings_linux::detail::readTermios2<int>(
                handle_context.file_descriptor, &serial_settings, error_callback) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Control::kGetStateError);
        }

        cpp_bindings_linux::detail::applyStopBits(&serial_settings, *stop_bits_value);

        if (cpp_bindings_linux::detail::writeTermios2<int>(
                handle_context.file_descriptor, &serial_settings, error_callback,
                cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetStopBitsError)) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Configuration::kSetStopBitsError);
        }

        tcflush(handle_context.file_descriptor, TCIOFLUSH);

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
