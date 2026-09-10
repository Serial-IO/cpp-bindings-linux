#include <cpp_core/interface/serial_open.h>
#include <cpp_core/validation.hpp>

#include "detail/apply_baudrate.hpp"
#include "detail/apply_data_bits.hpp"
#include "detail/apply_flow_control.hpp"
#include "detail/apply_parity.hpp"
#include "detail/apply_stop_bits.hpp"
#include "detail/effective_error_callback.hpp"
#include "detail/fail_errno.hpp"
#include "detail/handle_types.hpp"
#include "detail/read_termios2.hpp"
#include "detail/register_opened_handle.hpp"
#include "detail/status_value.hpp"
#include "detail/termios2.hpp"
#include "detail/write_termios2.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

extern "C"
{

    MODULE_API auto serialOpen(const char *port, const cpp_core::SerialConfig *config, ErrorCallbackT error_callback)
        -> intptr_t
    {
        const auto callback = cpp_bindings_linux::detail::effectiveErrorCallback(error_callback);
        const auto validation_rc = cpp_core::validateOpenParams<intptr_t>(port, config, callback);
        if (validation_rc < 0)
        {
            return validation_rc;
        }

        cpp_bindings_linux::detail::UniqueFd handle(open(port, O_RDWR | O_NOCTTY | O_NONBLOCK));
        if (!handle.valid())
        {
            return cpp_bindings_linux::detail::failErrno<intptr_t>(
                error_callback,
                cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Connection::kNotFoundError));
        }

        termios2 serial_settings{};
        if (cpp_bindings_linux::detail::readTermios2<int>(handle.get(), &serial_settings, error_callback) < 0)
        {
            return static_cast<intptr_t>(cpp_core::StatusCode::Control::kGetStateError);
        }

        cpp_bindings_linux::detail::applyBaudrate(&serial_settings, config->baudrate);
        cpp_bindings_linux::detail::applyDataBits(&serial_settings, cpp_core::toInt(config->data_bits));
        cpp_bindings_linux::detail::applyParity(&serial_settings, config->parity);
        cpp_bindings_linux::detail::applyStopBits(&serial_settings, config->stop_bits);

        serial_settings.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        serial_settings.c_iflag &= ~(IXON | IXOFF | IXANY | INLCR | IGNCR | ICRNL);
        serial_settings.c_oflag &= ~OPOST;
        cpp_bindings_linux::detail::applyFlowControl(&serial_settings, config->flow_mode);
        serial_settings.c_cc[VMIN] = 0;
        serial_settings.c_cc[VTIME] = 0;

        if (cpp_bindings_linux::detail::writeTermios2<int>(
                handle.get(), &serial_settings, error_callback,
                cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kSetStateError)) < 0)
        {
            return static_cast<intptr_t>(cpp_core::StatusCode::Control::kSetStateError);
        }

        tcflush(handle.get(), TCIOFLUSH);

        const int raw_fd = handle.release();
        cpp_bindings_linux::detail::registerOpenedHandle(raw_fd);
        return static_cast<intptr_t>(raw_fd);
    }

} // extern "C"
