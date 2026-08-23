#pragma once

#include <cpp_core/error_callback.h>
#include <cpp_core/serial_config.hpp>
#include <cpp_core/status_code.h>

#include <atomic>

namespace cpp_bindings_linux::detail
{
using IoCallbackT = void (*)(int);
using StatusCodeValue = cpp_core::StatusCodeValue;
using cpp_core::FlowControl;
using cpp_core::Parity;
using cpp_core::StatusCode;
using cpp_core::StopBits;

inline std::atomic<ErrorCallbackT> g_error_callback{nullptr};
} // namespace cpp_bindings_linux::detail
