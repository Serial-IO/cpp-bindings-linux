#include <cpp_core/interface/serial_set_data_bits.h>

#include "detail/posix_acquire_handle_context.hpp"
#include "detail/posix_apply_data_bits.hpp"
#include "detail/posix_fail_validation.hpp"
#include "detail/posix_read_termios2.hpp"
#include "detail/posix_status_value.hpp"
#include "detail/posix_validate_data_bits_value.hpp"
#include "detail/posix_write_termios2.hpp"
#include "detail/posix_termios2.hpp"

#include <sys/ioctl.h>
#include <termios.h>

extern "C"
{

    MODULE_API auto serialSetDataBits(int64_t handle, int data_bits, ErrorCallbackT error_callback) -> int
    {
        cpp_bindings_linux::detail::HandleContext handle_context;
        const auto status =
            cpp_bindings_linux::detail::acquireHandleContext<int>(handle, error_callback, &handle_context);
        if (status < 0)
        {
            return status;
        }

        if (!cpp_bindings_linux::detail::validateDataBitsValue(data_bits))
        {
            return cpp_bindings_linux::detail::failValidation<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetDataBitsError));
        }

        termios2 serial_settings{};
        if (cpp_bindings_linux::detail::readTermios2<int>(
                handle_context.file_descriptor, &serial_settings, error_callback) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Control::kGetStateError);
        }

        cpp_bindings_linux::detail::applyDataBits(&serial_settings, data_bits);

        if (cpp_bindings_linux::detail::writeTermios2<int>(
                handle_context.file_descriptor, &serial_settings, error_callback,
                cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Configuration::kSetDataBitsError)) < 0)
        {
            return static_cast<int>(cpp_core::StatusCode::Configuration::kSetDataBitsError);
        }

        tcflush(handle_context.file_descriptor, TCIOFLUSH);

        return static_cast<int>(cpp_core::StatusCode::kSuccess);
    }

} // extern "C"
