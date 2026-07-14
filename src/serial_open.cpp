#include <cpp_core/interface/serial_open.h>
#include <cpp_core/validation.hpp>

#include "detail/posix_apply_baudrate.hpp"
#include "detail/posix_apply_data_bits.hpp"
#include "detail/posix_apply_parity.hpp"
#include "detail/posix_apply_stop_bits.hpp"
#include "detail/posix_effective_error_callback.hpp"
#include "detail/posix_fail_errno.hpp"
#include "detail/posix_handle_types.hpp"
#include "detail/posix_parse_parity.hpp"
#include "detail/posix_parse_stop_bits.hpp"
#include "detail/posix_read_termios2.hpp"
#include "detail/posix_register_opened_handle.hpp"
#include "detail/posix_status_value.hpp"
#include "detail/posix_write_termios2.hpp"
#include "detail/posix_termios2.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

extern "C"
{

    MODULE_API auto serialOpen(void *port, int baudrate, int data_bits, int parity, int stop_bits,
                               ErrorCallbackT error_callback) -> intptr_t
    {
        const auto callback = cpp_bindings_linux::detail::effectiveErrorCallback(error_callback);
        const auto validation_rc = cpp_core::validateOpenParams<intptr_t>(port, baudrate, data_bits, callback);
        if (validation_rc < 0)
        {
            return validation_rc;
        }

        const auto parity_value = cpp_bindings_linux::detail::parseParity(
            parity, error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kSetStateError));
        if (!parity_value.has_value())
        {
            return static_cast<intptr_t>(cpp_core::StatusCode::Control::kSetStateError);
        }

        const auto stop_bits_value = cpp_bindings_linux::detail::parseStopBits(
            stop_bits, error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Control::kSetStateError));
        if (!stop_bits_value.has_value())
        {
            return static_cast<intptr_t>(cpp_core::StatusCode::Control::kSetStateError);
        }

        const char *port_path = static_cast<const char *>(port);
        cpp_bindings_linux::detail::UniqueFd handle(open(port_path, O_RDWR | O_NOCTTY | O_NONBLOCK));
        if (!handle.valid())
        {
            return cpp_bindings_linux::detail::failErrno<intptr_t>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Connection::kNotFoundError));
        }

        termios2 serial_settings{};
        if (cpp_bindings_linux::detail::readTermios2<int>(handle.get(), &serial_settings, error_callback) < 0)
        {
            return static_cast<intptr_t>(cpp_core::StatusCode::Control::kGetStateError);
        }

        cpp_bindings_linux::detail::applyBaudrate(&serial_settings, baudrate);
        cpp_bindings_linux::detail::applyDataBits(&serial_settings, data_bits);
        cpp_bindings_linux::detail::applyParity(&serial_settings, *parity_value);
        cpp_bindings_linux::detail::applyStopBits(&serial_settings, *stop_bits_value);

        serial_settings.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        serial_settings.c_iflag &= ~(IXON | IXOFF | IXANY | INLCR | IGNCR | ICRNL);
        serial_settings.c_oflag &= ~OPOST;
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
