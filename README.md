# C++ Bindings for Linux

[![Build](https://github.com/Serial-IO/cpp-bindings-linux/actions/workflows/build_binary.yml/badge.svg)](https://github.com/Serial-IO/cpp-bindings-linux/actions/workflows/build_binary.yml)
[![JSR](https://jsr.io/badges/@serial/cpp-bindings-linux)](https://jsr.io/@serial/cpp-bindings-linux)

Linux shared library for serial communication. It implements the
[`cpp-core`](https://github.com/Serial-IO/cpp-core) interface and provides functions for discovering, opening,
configuring, reading from, and writing to serial ports.

## Requirements

- Linux
- Git
- CMake 3.30 or newer
- Ninja
- A compiler with C++26 support (CI uses GCC 16)

CMake downloads `cpp-core` and GoogleTest automatically during configuration.

## Build

```sh
git clone https://github.com/Serial-IO/cpp-bindings-linux.git
cd cpp-bindings-linux
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release --target cpp_bindings_linux
```

The shared library is written to `build/libcpp_bindings_linux.so`.

To select a specific compiler, add it while configuring, for example:

```sh
cmake --preset linux-gcc-release \
  -DCMAKE_C_COMPILER=gcc-16 \
  -DCMAKE_CXX_COMPILER=g++-16
```

## Tests

Build and run the C++ test suite:

```sh
cmake --build --preset linux-gcc-release --target cpp_bindings_linux_tests
ctest --test-dir build --output-on-failure
```

Tests that require a serial device use `SERIAL_TEST_PORT`. They are skipped when no suitable device is available.

The optional Deno FFI smoke tests require Deno 2 and a built library:

```sh
cd integration_tests
deno task test
```

## License

This project is licensed under the [GNU Lesser General Public License v3.0](LICENSE).
