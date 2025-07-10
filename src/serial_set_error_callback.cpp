#include "serial_internal.hpp"

#include <cpp_core/interface/serial_set_error_callback.h>

extern "C" void serialSetErrorCallback(ErrorCallbackT error_callback)
{
    serial_internal::g_error_callback = error_callback;
}
