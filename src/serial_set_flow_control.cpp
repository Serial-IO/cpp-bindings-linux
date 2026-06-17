#include <cpp_core/interface/serial_set_flow_control.h>

#include "detail/posix_helpers.hpp"
#include "detail/posix_termios2.hpp"

#include <sys/ioctl.h>

extern "C"
{

    MODULE_API auto serialSetFlowControl(int64_t handle, int mode, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext context;
        const auto rc = cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &context);
        if (rc < 0)
        {
            return rc;
        }

        const auto flow_control = cpp_bindings_linux::detail::parseFlowControl(
            mode, error_callback,
            cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetFlowControlError));
        if (!flow_control.has_value())
        {
            return static_cast<int>(cpp_core::StatusCode::Configuration::kSetFlowControlError);
        }

        termios2 tty{};
        if (cpp_bindings_linux::detail::readTermios2<int>(context.fd, &tty, error_callback) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Control::kGetStateError);
        }

        cpp_bindings_linux::detail::applyFlowControl(&tty, *flow_control);

        if (cpp_bindings_linux::detail::writeTermios2<int>(
                context.fd, &tty, error_callback,
                cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetFlowControlError)) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Configuration::kSetFlowControlError);
        }

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
