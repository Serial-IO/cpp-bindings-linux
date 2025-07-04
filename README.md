# cpp_unix_bindings

A compact C++23 library for talking to serial devices on Linux (e.g. Arduino).  
The project builds a **shared library `libcpp_unix_bindings.so`** that can be used via
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
#   build/libcpp_unix_bindings.so
```

> Because `CMAKE_EXPORT_COMPILE_COMMANDS` is enabled, the build also generates a
> `compile_commands.json` that is automatically copied to the project root —
> handy for clang-tools (clangd, clang-tidy, …).

---

## 2  Using the library from **Deno** (v1.42 or newer)

Deno ships with a first-class FFI API.

```ts
// serial_deno.ts
const lib = Deno.dlopen('./build/libcpp_unix_bindings.so', {
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

Below is a condensed overview of the most relevant functions. See `serial.h` for full
signatures and additional helpers.

### Connection
* `serialOpen(...)` – open a serial device and return a handle
* `serialClose(handle)` – close the device

### I/O
* `serialRead(...)` / `serialWrite(...)` – basic read/write with timeout
* `serialReadUntil(...)` – read until a specific char is encountered (inclusive)
* `serialReadLine(...)` – read until `\n`
* `serialWriteLine(...)` – write buffer and append `\n`
* `serialReadUntilToken(...)` – read until a string token is encountered
* `serialReadFrame(...)` – read a frame delimited by start & end bytes

### Helpers
* `serialPeek(...)` – look at the next byte without consuming it
* `serialDrain(...)` – wait until all TX bytes are sent
* `serialClearBufferIn/Out(...)` – drop buffered bytes
* `serialAbortRead/Write(...)` – abort pending I/O operations

### Statistics
* `serialGetRxBytes(handle)` / `serialGetTxBytes(handle)` – cumulative RX / TX byte counters

### Enumeration & autodetect
* `serialGetPortsInfo(...)` – list available ports under `/dev/serial/by-id`

### Callbacks
* `serialOnError(func)` – error callback
* `serialOnRead(func)` – read callback (bytes read)
* `serialOnWrite(func)` – write callback (bytes written)

Return values ≤ 0 correspond to error codes defined in `status_codes.h`.

---

## 4  Ready-made Deno examples

Two runnable scripts live in `examples/` and require only Deno plus the compiled
shared library.

- **serial_echo.ts** – Minimal echo test that lists available ports, opens the
  first one and verifies that the micro-controller echoes the sent string.
  Run it with:
  ```bash
  deno run --allow-ffi --allow-read examples/serial_echo.ts \
           --lib ./build/libcpp_unix_bindings.so \
           --port /dev/ttyUSB0
  ```

- **serial_advanced.ts** – Shows the high-level helpers (`serialWriteLine`,
  `serialReadLine`, `serialPeek`, `statistics`, `serialDrain`). It sends three
  lines and then prints the TX/RX counters.
  ```bash
  deno run --allow-ffi --allow-read examples/serial_advanced.ts \
           --lib ./build/libcpp_unix_bindings.so \
           --port /dev/ttyUSB0
  ```

Notes:
1. `--lib` defaults to `./build/libcpp_unix_bindings.so`; pass a custom path if
you installed the library elsewhere.
2. `--port` defaults to `/dev/ttyUSB0`; adjust if your board shows up under a
different device (e.g. `/dev/ttyACM0`).
3. On most Arduino boards opening the port toggles DTR and triggers a reset.
   Both scripts therefore wait ~2 s after opening the device before sending the
   first command.

---
