# C API Reference

**Scope** – Public interface of `libcpp_unix_bindings` (C-linkable).  
*Linux fully supported · Windows fully supported*

Every function is declared in `src/serial.h` and follows the same
convention: **positive / non-zero** ⇒ success, **≤ 0** ⇒ failure (maps to
`StatusCodes`).

---

## 1 · Connection

```c
intptr_t serialOpen(void* port, int baud, int dataBits,
                    int parity /*0-none·1-even·2-odd*/,
                    int stopBits /*0-1bit·2-2bit*/);

void serialClose(int64_t handle);
```

*Linux*: `port` is a NULL-terminated path like `/dev/ttyUSB0` or
`/dev/tty.SLAB_USBtoUART`.  
*Windows*: use `"COM3"`, `"\\.\\COM12"`, etc. (to be implemented).

---

## 2 · Basic I/O

```c
int serialRead (int64_t handle, void* buf, int len, int timeoutMs, int mult);
int serialWrite(int64_t handle, const void* buf, int len, int timeoutMs, int mult);

int serialReadUntil(int64_t handle, void* buf, int len,
                    int timeoutMs, int mult, void* untilCharPtr);
```

*mult* is a polling multiplier (usually **1**). `serialReadUntil` stops **after**
the delimiter byte has been copied into *buf*.

---

## 3 · Convenience helpers

```c
int serialReadLine (int64_t h, void* buf, int len, int timeoutMs);          // until '\n'
int serialWriteLine(int64_t h, const void* buf, int len, int timeoutMs);     // appends '\n'

int serialReadUntilSequence(int64_t h, void* buf, int len,
                            int timeoutMs, void* sequencePtr);

int serialReadFrame(int64_t h, void* buf, int len, int timeoutMs,
                    char startByte, char endByte);
```

---

## 4 · Statistics & Buffer control

```c
int64_t serialInBytesTotal (int64_t handle);   // cumulative RX
int64_t serialOutBytesTotal(int64_t handle);   // cumulative TX

int serialInBytesWaiting (int64_t handle);     // queued in driver
int serialOutBytesWaiting(int64_t handle);

int  serialDrain(int64_t handle);              // wait for TX empty

void serialClearBufferIn (int64_t handle);
void serialClearBufferOut(int64_t handle);

void serialAbortRead (int64_t handle);
void serialAbortWrite(int64_t handle);
```

---

## 5 · Port enumeration & Callbacks

```c
// Enumerate available ports. For each port the callback receives:
//   portPath, aliasPath, manufacturer, serialNumber, pnpId,
//   locationId, productId, vendorId
int serialGetPortsInfo(void (*cb)(const char*, const char*, const char*,
                                  const char*, const char*, const char*,
                                  const char*, const char*));

void serialOnError(void (*cb)(int code, const char* msg));
void serialOnRead (void (*cb)(int bytes));
void serialOnWrite(void (*cb)(int bytes));
```

*Linux*: implementation scans `/dev/serial/by-id` if present, otherwise falls
back to common tty names.  
*Windows*: will iterate over available COM ports.

---

## 6 · StatusCodes

```c++
enum class StatusCodes : int {
    SUCCESS                 =  0,
    CLOSE_HANDLE_ERROR      = -1,
    INVALID_HANDLE_ERROR    = -2,
    READ_ERROR              = -3,
    WRITE_ERROR             = -4,
    GET_STATE_ERROR         = -5,
    SET_STATE_ERROR         = -6,
    SET_TIMEOUT_ERROR       = -7,
    BUFFER_ERROR            = -8,
    NOT_FOUND_ERROR         = -9,
    CLEAR_BUFFER_IN_ERROR   = -10,
    CLEAR_BUFFER_OUT_ERROR  = -11,
    ABORT_READ_ERROR        = -12,
    ABORT_WRITE_ERROR       = -13
};
```

---

### Error-handling idiom

```c
int rv = serialWrite(h, data, len, 500, 1);
if (rv <= 0) {
    // handle error – detailed info via registered onError callback (if any)
}
```
