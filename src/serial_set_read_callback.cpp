#include "serial_internal.hpp"

#include <cpp_core/interface/serial_set_read_callback.h>

extern "C" void serialSetReadCallback(void (*callback_fn)(int))
{
    serial_internal::g_read_callback = callback_fn;
}
