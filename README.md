# CPP-Unix-Bindings

A compact C++23 library for talking to serial devices on Linux (e.g. Arduino).  
The project builds a **shared library `libCPP-Unix-Bindings.so`** that can be used via
Deno's native FFI.

---

## 1  Building the library

```bash
# Clone
git clone https://github.com/Serial-IO/cpp-bindings-unix.git cpp-bindings-unix
cd cpp-bindings-unix

# Dependencies (Debian/Ubuntu)
sudo apt-get install build-essential cmake clang make

# Compile
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# The resulting library will be located at
#   build/libCPP-Unix-Bindings.so
```

> Because `CMAKE_EXPORT_COMPILE_COMMANDS` is enabled, the build also generates a
> `compile_commands.json` that is automatically copied to the project root —
> handy for clang-tools (clangd, clang-tidy, …).

---

## 2  Using the library from **Deno** (v1.42 or newer)

Deno ships with a first-class FFI API.

```ts
// serial_deno.ts
const lib = Deno.dlopen('./build/libCPP-Unix-Bindings.so', {
  serialOpen: { parameters: [ 'buffer', 'i32', 'i32', 'i32', 'i32' ], result: 'pointer' },
  serialClose: { parameters: [ 'pointer' ], result: 'void' },
  serialRead: { parameters: [ 'pointer', 'buffer', 'i32', 'i32', 'i32' ], result: 'i32' },
  serialWrite: { parameters: [ 'pointer', 'buffer', 'i32', 'i32', 'i32' ], result: 'i32' },
});

const enc = new TextEncoder();
const dec = new TextDecoder();

// Note: device path must be null-terminated
const handle = lib.symbols.serialOpen(enc.encode('/dev/ttyUSB0\0'), 115200, 8, 0, 0);

const writeBuf = enc.encode('Hello\n');
lib.symbols.serialWrite(handle, writeBuf, writeBuf.length, 100, 1);

const readBuf = new Uint8Array(128);
const n = lib.symbols.serialRead(handle, readBuf, readBuf.length, 500, 1);
console.log(dec.decode(readBuf.subarray(0, n)));

lib.symbols.serialClose(handle);
lib.close();
```

---

## 3  C API reference

| Function | Description |
|----------|-------------|
| `intptr_t serialOpen(const char* dev, int baud, int bits, int parity, int stop)` | Open a device and return a handle. |
| `void serialClose(intptr_t handle)` | Close the port. |
| `int serialRead(...)` | Read bytes with timeout. |
| `int serialWrite(...)` | Write bytes with timeout. |
| `int serialGetPortsInfo(char* buffer, int len, const char* sep)` | List ports under `/dev/serial/by-id`. |
| `void serialOnError(void (*)(int))` | Register an error callback. |
| *(others in `serial.h`)* |

Return values ≤ 0 indicate error codes defined in `status_codes.h`.

---
