#pragma once
#include <cstdint>

#define MODULE_API __attribute__((visibility("default")))

#ifdef __cplusplus
extern "C"
{
#endif

    // Version helpers generated at configure time
    MODULE_API unsigned int getMajorVersion();
    MODULE_API unsigned int getMinorVersion();
    MODULE_API unsigned int getPatchVersion();

    // Basic serial API
    MODULE_API intptr_t
    serialOpen(void* port, int baudrate, int dataBits, int parity /*0-none,1-even,2-odd*/ = 0, int stopBits /*0-1bit,2-2bit*/ = 0);

    MODULE_API void serialClose(int64_t handle);

    MODULE_API int serialRead(int64_t handle, void* buffer, int bufferSize, int timeout /*ms*/, int multiplier);

    MODULE_API int serialReadUntil(int64_t handle, void* buffer, int bufferSize, int timeout, int multiplier, void* untilChar);

    MODULE_API int serialWrite(int64_t handle, const void* buffer, int bufferSize, int timeout, int multiplier);

    MODULE_API int serialGetPortsInfo(void* buffer, int bufferSize, void* separator);

    MODULE_API void serialClearBufferIn(int64_t handle);
    MODULE_API void serialClearBufferOut(int64_t handle);
    MODULE_API void serialAbortRead(int64_t handle);
    MODULE_API void serialAbortWrite(int64_t handle);

    // Optional callback hooks (can be nullptr)
    extern void (*error_callback)(int errorCode);
    extern void (*read_callback)(int bytes);
    extern void (*write_callback)(int bytes);

    MODULE_API void serialOnError(void (*func)(int code));
    MODULE_API void serialOnRead(void (*func)(int bytes));
    MODULE_API void serialOnWrite(void (*func)(int bytes));

#ifdef __cplusplus
}
#endif
