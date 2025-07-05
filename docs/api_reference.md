# C API Reference

All functions are declared in `src/serial.h`. **Return values ≤ 0** indicate an error; the numeric code maps to `StatusCodes` (see bottom).

---

## Connection

| Function | Description |
|----------|-------------|
| `intptr_t serialOpen(void* port, int baud, int dataBits, int parity, int stopBits)` | Open a device (e.g. `"/dev/ttyUSB0\0"`) and receive an opaque handle. Parity: `0` = none, `1` = even, `2` = odd. Stop bits: `0` = 1 bit, `2` = 2 bits. |
| `void serialClose(int64_t handle)` | Restore original tty settings and close the file descriptor. |

## Basic I/O

| Function | Description |
|----------|-------------|
| `int serialRead(handle, buf, len, timeoutMs, multiplier)` | Read up to *len* bytes. Returns actual bytes read or `0` on timeout. |
| `int serialWrite(handle, buf, len, timeoutMs, multiplier)` | Write *len* bytes and return the number of bytes written. |
| `int serialReadUntil(handle, buf, len, timeoutMs, multiplier, untilCharPtr)` | Read until the delimiter byte (inclusive). |

## Extended helpers

| Function | Description |
|----------|-------------|
| `int serialReadLine(handle, buf, len, timeoutMs)` | Read until `\n`. |
| `int serialWriteLine(handle, buf, len, timeoutMs)` | Write buffer followed by `\n`. |
| `int serialReadUntilSequence(handle, buf, len, timeoutMs, seqPtr)` | Read until the UTF-8 sequence is encountered. |
| `int serialReadFrame(handle, buf, len, timeoutMs, startByte, endByte)` | Read a frame delimited by *startByte* / *endByte*. |

## Statistics & Buffer control

| Function | Description |
|----------|-------------|
| `int64_t serialInBytesTotal(handle)` / `serialOutBytesTotal(handle)` | Cumulative RX / TX counters. |
| `int serialInBytesWaiting(handle)` / `serialOutBytesWaiting(handle)` | Bytes currently queued in driver buffers. |
| `int serialDrain(handle)` | Block until OS transmit buffer is empty. |
| `void serialClearBufferIn/Out(handle)` | Flush RX / TX buffers. |
| `void serialAbortRead/Write(handle)` | Cancel in-flight I/O from another thread. |

## Enumeration & Callbacks

| Function | Description |
|----------|-------------|
| `int serialGetPortsInfo(cb)` | Call *cb* for every entry under `/dev/serial/by-id`. |
| `void serialOnError(cb)` | Register global error callback. |
| `void serialOnRead(cb)` / `serialOnWrite(cb)` | Called after each successful read/write. |

---

## StatusCodes

| Constant | Value | Meaning |
|----------|------:|---------|
| `SUCCESS` | 0 | No error |
| `CLOSE_HANDLE_ERROR` | -1 | Closing the device failed |
| `INVALID_HANDLE_ERROR` | -2 | Handle was null / invalid |
| `READ_ERROR` | -3 | `read()` syscall failed |
| `WRITE_ERROR` | -4 | `write()` syscall failed |
| `GET_STATE_ERROR` | -5 | Could not query tty state |
| `SET_STATE_ERROR` | -6 | Could not set tty state |
| `SET_TIMEOUT_ERROR` | -7 | Could not configure timeout |
| `BUFFER_ERROR` | -8 | Internal buffer issue |
| `NOT_FOUND_ERROR` | -9 | Resource not found (`/dev/serial/by-id`) |
| `CLEAR_BUFFER_IN_ERROR` | -10 | Flushing RX failed |
| `CLEAR_BUFFER_OUT_ERROR` | -11 | Flushing TX failed |
| `ABORT_READ_ERROR` | -12 | Abort flag could not be set |
| `ABORT_WRITE_ERROR` | -13 | Abort flag could not be set | 
