#include <cpp_core/interface/serial_set_flow_control.h>

#include "detail/posix_acquire_handle_context.hpp"
#include "detail/posix_apply_flow_control.hpp"
#include "detail/posix_parse_flow_control.hpp"
#include "detail/posix_read_termios2.hpp"
#include "detail/posix_status_value.hpp"
#include "detail/posix_write_termios2.hpp"
#include "detail/posix_termios2.hpp"

#include <sys/ioctl.h>
#include <termios.h>

extern "C"
{

    MODULE_API auto serialSetFlowControl(int64_t handle, int mode, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext handle_context;
        const auto status =
            cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &handle_context);
        if (status < 0)
        {
            return status;
        }

        const auto flow_control = cpp_bindings_linux::detail::parseFlowControl(
            mode, error_callback,
            cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetFlowControlError));
        if (!flow_control.has_value())
        {
            return static_cast<int>(cpp_core::StatusCode::Configuration::kSetFlowControlError);
        }

        termios2 serial_settings{};
        if (cpp_bindings_linux::detail::readTermios2<int>(
                handle_context.file_descriptor, &serial_settings, error_callback) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Control::kGetStateError);
        }

        cpp_bindings_linux::detail::applyFlowControl(&serial_settings, *flow_control);

        if (cpp_bindings_linux::detail::writeTermios2<int>(
                handle_context.file_descriptor, &serial_settings, error_callback,
                cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetFlowControlError)) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Configuration::kSetFlowControlError);
        }

        tcflush(handle_context.file_descriptor, TCIOFLUSH);

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
