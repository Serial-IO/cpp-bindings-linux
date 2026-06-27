#pragma once

#include "posix_common.hpp"
#include "posix_termios2.hpp"

#include <cpp_core/serial_config.hpp>

#include <optional>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>

namespace cpp_bindings_linux::detail
{
inline auto validateBaudrateValue(int baudrate) -> cpp_core::Status
{
    if (cpp_core::SerialConfig::tryMake(baudrate, 8))
    {
        return cpp_core::ok();
    }

    return cpp_core::fail<>(statusValue(StatusCode::Configuration::kSetBaudrateError),
                            std::string(defaultValidationMessage(
                                statusValue(StatusCode::Configuration::kSetBaudrateError))));
}

inline auto validateDataBitsValue(int data_bits) -> cpp_core::Status
{
    if (cpp_core::SerialConfig::tryMake(300, data_bits))
    {
        return cpp_core::ok();
    }

    return cpp_core::fail<>(statusValue(StatusCode::Configuration::kSetDataBitsError),
                            std::string(defaultValidationMessage(
                                statusValue(StatusCode::Configuration::kSetDataBitsError))));
}

inline auto parseParity(int parity, ErrorCallbackT error_callback, StatusCodeValue invalid_code)
    -> std::optional<Parity>
{
    switch (parity)
    {
    case 0:
        return Parity::kNone;
    case 1:
        return Parity::kEven;
    case 2:
        return Parity::kOdd;
    default:
        (void)failValidation<int>(error_callback, invalid_code);
        return std::nullopt;
    }
}

inline auto parseStopBits(int stop_bits, ErrorCallbackT error_callback, StatusCodeValue invalid_code,
                          bool allow_one_alias = true) -> std::optional<StopBits>
{
    switch (stop_bits)
    {
    case 0:
        return StopBits::kOne;
    case 1:
        if (allow_one_alias)
        {
            return StopBits::kOne;
        }
        break;
    case 2:
        return StopBits::kTwo;
    default:
        break;
    }

    (void)failValidation<int>(error_callback, invalid_code);
    return std::nullopt;
}

inline auto parseFlowControl(int mode, ErrorCallbackT error_callback, StatusCodeValue invalid_code)
    -> std::optional<FlowControl>
{
    switch (mode)
    {
    case 0:
        return FlowControl::kNone;
    case 1:
        return FlowControl::kRtsCts;
    case 2:
        return FlowControl::kXonXoff;
    default:
        (void)failValidation<int>(error_callback, invalid_code);
        return std::nullopt;
    }
}

template <typename Ret>
inline auto readTermios2(int file_descriptor, termios2 *serial_settings, ErrorCallbackT error_callback) -> Ret
{
    if (ioctl(file_descriptor, TCGETS2, serial_settings) != 0)
    {
        return failErrno<Ret>(error_callback, statusValue(StatusCode::Control::kGetStateError));
    }

    return static_cast<Ret>(StatusCode::kSuccess);
}

template <typename Ret>
inline auto writeTermios2(int file_descriptor, termios2 *serial_settings, ErrorCallbackT error_callback,
                          StatusCodeValue set_error_code) -> Ret
{
    if (ioctl(file_descriptor, TCSETS2, serial_settings) != 0)
    {
        return failErrno<Ret>(error_callback, set_error_code);
    }

    return static_cast<Ret>(StatusCode::kSuccess);
}

inline auto applyBaudrate(termios2 *serial_settings, int baudrate) -> void
{
    serial_settings->c_cflag &= ~CBAUD;
    serial_settings->c_cflag |= BOTHER;
    serial_settings->c_ispeed = static_cast<speed_t>(baudrate);
    serial_settings->c_ospeed = static_cast<speed_t>(baudrate);
}

inline auto applyDataBits(termios2 *serial_settings, int data_bits) -> void
{
    serial_settings->c_cflag &= ~CSIZE;
    switch (data_bits)
    {
    case 5:
        serial_settings->c_cflag |= CS5;
        break;
    case 6:
        serial_settings->c_cflag |= CS6;
        break;
    case 7:
        serial_settings->c_cflag |= CS7;
        break;
    case 8:
    default:
        serial_settings->c_cflag |= CS8;
        break;
    }
}

inline auto applyParity(termios2 *serial_settings, Parity parity) -> void
{
    serial_settings->c_cflag &= ~(PARENB | PARODD);
    if (parity == Parity::kEven)
    {
        serial_settings->c_cflag |= PARENB;
    }
    else if (parity == Parity::kOdd)
    {
        serial_settings->c_cflag |= (PARENB | PARODD);
    }
}

inline auto applyStopBits(termios2 *serial_settings, StopBits stop_bits) -> void
{
    if (stop_bits == StopBits::kTwo)
    {
        serial_settings->c_cflag |= CSTOPB;
    }
    else
    {
        serial_settings->c_cflag &= ~CSTOPB;
    }
}

inline auto applyFlowControl(termios2 *serial_settings, FlowControl flow_control) -> void
{
    serial_settings->c_cflag &= ~CRTSCTS;
    serial_settings->c_iflag &= ~(IXON | IXOFF | IXANY);

    if (flow_control == FlowControl::kRtsCts)
    {
        serial_settings->c_cflag |= CRTSCTS;
    }
    else if (flow_control == FlowControl::kXonXoff)
    {
        serial_settings->c_iflag |= (IXON | IXOFF);
    }
}

} // namespace cpp_bindings_linux::detail
