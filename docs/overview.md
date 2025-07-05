# cpp_unix_bindings – Overview

`cpp_unix_bindings` is a compact C++23 shared library that exposes a clean C API for communicating with serial devices on Linux / POSIX.  It is designed to be consumed from high-level runtimes (most notably **Deno**) through a thin FFI layer.

## Key features

* Header-only C API – no C++ ABI headaches
* Zero external dependencies beyond the standard C library and POSIX (`termios.h`)
* Robust timeout-aware read / write helpers
* High-level convenience helpers (`serialReadLine`, framed reads, token reads …)
* Built-in port enumeration under `/dev/serial/by-id`
* Optional callbacks for errors, read & write statistics
* Thread-safe, lock-free I/O abort (`serialAbortRead/Write`)
* Accurate TX/RX byte counters for live statistics

The repository contains:

* `src/` – core C++ implementation
* `examples/` – runnable Deno scripts demonstrating usage
* `tests/` – GoogleTest based C++ unit tests
* `docs/` – in-depth documentation (this directory)

Supported platforms: **Linux** on x86_64 / ARM.  macOS should compile with minor tweaks but is currently untested. 
