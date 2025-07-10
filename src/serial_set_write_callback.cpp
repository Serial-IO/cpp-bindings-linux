#include "serial_internal.hpp"

#include <cpp_core/interface/serial_set_write_callback.h>

extern "C" void serialSetWriteCallback(void (*callback_fn)(int))
{
    serial_internal::g_write_callback = callback_fn;
}
