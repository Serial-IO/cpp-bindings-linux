# C++ Bindings for Linux

[![Build](https://github.com/Serial-IO/cpp-bindings-linux/actions/workflows/build_binary.yml/badge.svg)](https://github.com/Serial-IO/cpp-bindings-linux/actions/workflows/build_binary.yml)
[![JSR](https://jsr.io/badges/@serial/cpp-bindings-linux)](https://jsr.io/@serial/cpp-bindings-linux)

Runtime-agnostic Linux shared library for serial communication. It implements
the [`cpp-core`](https://github.com/Serial-IO/cpp-core) interface and provides
functions for discovering, opening, configuring, reading from, and writing to
serial ports.

The library exposes a C-compatible ABI and can be used from any language or
runtime that can load a GNU/Linux shared library and call C functions. Release
artifacts include machine-readable FFI metadata for generating runtime-specific
adapters, including exported symbols, types, callbacks, structs, defaults, and
API documentation.

## Requirements

- Linux
- Git
- CMake 3.30 or newer
- Ninja
- A compiler with sufficient C++26 support

CMake downloads `cpp-core` **v3.0.0** and GoogleTest automatically during configuration.

## Build

```sh
git clone https://github.com/Serial-IO/cpp-bindings-linux.git
cd cpp-bindings-linux
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release --target cpp_bindings_linux
```

The shared library is written to `build/libcpp_bindings_linux.so`.

Official release and JSR artifacts are built for these GNU/Linux targets:

| Target | CPU baseline | Minimum glibc |
| --- | --- | --- |
| `x86_64-linux-gnu` | generic x86-64 | 2.28 |
| `aarch64-linux-gnu` | ARMv8-A | 2.28 |

### Binary compatibility

The prebuilt binaries require **glibc 2.28 or newer**. Compatibility depends on
the installed glibc version rather than the distribution name. Common release
baselines are shown below for orientation:

| Distribution | Release baseline |
| --- | --- |
| Debian | 10+ |
| Ubuntu | 20.04 LTS+ |
| RHEL / Rocky Linux / AlmaLinux | 8+ |
| Fedora | 29+ |
| openSUSE Leap | 15.x (not compatible by default) |

Check the installed version with:

```sh
ldd --version
```

These versions indicate binary compatibility and do not imply that the listed
distribution releases are still supported by their vendors.

### Portable release builds

The release builds statically include the GNU C++ and compiler runtimes. They
still use the target system's glibc and therefore require glibc 2.28 or newer.
To reproduce the portable release configuration for the host architecture:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCPP_BINDINGS_LINUX_PORTABLE_CPU_BASELINE=ON \
  -DCPP_BINDINGS_LINUX_STATIC_CXX_RUNTIME=ON
cmake --build build --target cpp_bindings_linux
```

The CI release builds use pinned `manylinux_2_28` images with GCC 14. The FFI
metadata is generated separately with ASTrein 3.0.0 and a GCC 14 compile context.

To select a specific compiler, add it while configuring, for example:

```sh
cmake --preset linux-gcc-release \
  -DCMAKE_C_COMPILER=gcc-14 \
  -DCMAKE_CXX_COMPILER=g++-14
```

## cpp-core v3 API

This version implements the cpp-core v3 ABI. Rebuild native callers and regenerate
FFI adapters when upgrading from v2; the function signatures are incompatible.
The shared-library SONAME is `libcpp_bindings_linux.so.3`.

- `serialOpen` takes a `const char *` path and a `SerialConfig` pointer, including
  flow control. Data bits, parity, stop bits, and flow control use cpp-core enums
  in both configurations and the corresponding getters/setters.
- `serialRead` and `serialWrite` take byte buffers (`std::uint8_t *`, const for
  writes) and a `SerialTimeoutConfig` pointer. Null configurations, negative
  timeouts/multipliers, and overflowing timeout products return an error.
- `serialReadUntilSequence` takes a byte sequence and its explicit length,
  supporting embedded zero bytes. Use a one-byte newline or another terminator
  in place of the removed `serialReadLine` and `serialReadUntil` functions.
- `serialWaitForDrain` replaces `serialDrain`.
- `serialSetEventCallback` replaces `serialMonitorPorts`, with typed
  `PortEvent::kAttached` / `kDetached` notifications. Pass `nullptr` to stop.
- `meta` returns version and Git metadata for the loaded Linux binding library.

```cpp
#include <cpp_core/serial.h>

constexpr auto config =
    cpp_core::SerialConfig::make<9600, cpp_core::DataBits::kEight>();
constexpr auto timeout = cpp_core::SerialTimeoutConfig::make<100, 1>();
const auto handle = serialOpen("/dev/ttyUSB0", &config);
if (handle > 0) {
    std::uint8_t buffer[256];
    constexpr std::uint8_t newline[] = {'\n'};
    const int received = serialReadUntilSequence(
        handle, buffer, sizeof(buffer), &timeout, newline, sizeof(newline));
    // received >= 0: bytes read, including the terminator; < 0: error code.
    serialClose(handle);
}
```

`StopBits::kOne` has the ABI value **0** and `StopBits::kTwo` has value **2**;
value 1 is invalid. Enum getters encode failures as negative underlying integers,
which C++ callers can inspect using `cpp_core::toInt`.

## Tests

Build and run the C++ test suite:

```sh
cmake --build --preset linux-gcc-release --target cpp_bindings_linux_tests
ctest --test-dir build --output-on-failure
```

Tests that require a serial device use `SERIAL_TEST_PORT`. They are skipped when no suitable device is available.

The optional runtime integration smoke tests currently use Deno 2 as their FFI
test harness and require a built library. Deno is not required to consume the
library from another compatible runtime:

```sh
cd integration_tests
deno task test
```

## License

This project is licensed under the [GNU Lesser General Public License v3.0](LICENSE).
