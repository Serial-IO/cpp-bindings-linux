#include "serial_internal.hpp"

namespace serial_internal
{
ErrorCallbackT g_error_callback = nullptr;
void (*g_read_callback)(int)    = nullptr;
void (*g_write_callback)(int)   = nullptr;
} // namespace serial_internal
