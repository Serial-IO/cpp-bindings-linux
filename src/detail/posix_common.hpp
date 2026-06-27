#pragma once

#include <cpp_core/error_callback.h>
#include <cpp_core/error_handling.hpp>
#include <cpp_core/serial_config.hpp>
#include <cpp_core/status_code.h>
#include <cpp_core/validation.hpp>

#include <atomic>
#include <cerrno>
#include <string_view>
#include <system_error>

namespace cpp_bindings_linux::detail
{
using IoCallbackT = void (*)(int);
using StatusCodeValue = cpp_core::StatusCodeValue;
using cpp_core::FlowControl;
using cpp_core::Parity;
using cpp_core::StatusCode;
using cpp_core::StopBits;

inline std::atomic<ErrorCallbackT> g_error_callback{nullptr};

template <typename Code> constexpr auto statusValue(Code code) -> StatusCodeValue
{
    return static_cast<StatusCodeValue>(code);
}

inline auto effectiveErrorCallback(ErrorCallbackT error_callback) -> ErrorCallbackT
{
    return error_callback != nullptr ? error_callback : g_error_callback.load(std::memory_order_acquire);
}

inline auto defaultValidationMessage(StatusCodeValue code) -> std::string_view
{
    switch (code)
    {
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetBaudrateError):
        return "Invalid baudrate: must be >= 300";
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetDataBitsError):
        return "Invalid data bits: must be 5-8";
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetParityError):
        return "Invalid parity: must be 0, 1, or 2";
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetStopBitsError):
        return "Invalid stop bits: must be 0, 1, or 2";
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetFlowControlError):
        return "Invalid flow control mode: must be 0, 1, or 2";
    default:
        return "Invalid serial setting";
    }
}

template <typename Ret>
inline auto failMsg(ErrorCallbackT error_callback, StatusCodeValue code, std::string_view message) -> Ret
{
    return cpp_core::failMsg<Ret>(effectiveErrorCallback(error_callback), code, message);
}

template <typename Ret>
inline auto failErrno(ErrorCallbackT error_callback, StatusCodeValue code) -> Ret
{
    return cpp_core::failMsg<Ret>(effectiveErrorCallback(error_callback), code,
                                  std::error_code(errno, std::generic_category()).message());
}

template <typename Ret> inline auto failValidation(ErrorCallbackT error_callback, StatusCodeValue code) -> Ret
{
    return failMsg<Ret>(error_callback, code, defaultValidationMessage(code));
}

} // namespace cpp_bindings_linux::detail
