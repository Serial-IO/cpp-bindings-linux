# C++ Bindings Linux

[![Build Binary](https://github.com/Serial-IO/cpp-bindings-linux/actions/workflows/build_binary.yml/badge.svg)](https://github.com/Serial-IO/cpp-bindings-linux/actions/workflows/build_binary.yml)
[![JSR](https://jsr.io/badges/@serial/cpp-bindings-linux)](https://jsr.io/@serial/cpp-bindings-linux)

Binaries are provided as a
[package on JSR](https://jsr.io/@serial/cpp-bindings-linux). They are serialized
as a base64 string inside the JSON file.

The contained shared libraries and FFI metadata are runtime-agnostic. They can
be used by any language or runtime that can decode base64, write a file, load a
GNU/Linux shared library, and call its C ABI. The TypeScript exports are a
convenient distribution format, not a dependency on a particular runtime.

The package contains portable binaries for `x86_64-linux-gnu` and
`aarch64-linux-gnu`, both requiring glibc 2.28 or newer. The x86-64 artifact
uses the generic x86-64 baseline.

It also includes cpp-core FFI API metadata generated with
[ASTrein](https://github.com/Katze719/ASTrein) at `bin/x86_64/ffi.json` and
`bin/aarch64/ffi.json`. It describes the exported C symbols, parameter and
return types, callbacks, structs, default values, and API documentation used by
runtime-specific FFI adapter generators.

This package is primarily intended as a dependency for
[`@serial/serial`](https://jsr.io/@serial/serial). However, it can also be used
independently.

## Usage

Select the export matching the host architecture. Each export contains the
base64-encoded shared library and its matching FFI metadata:

```ts
import { aarch64, x86_64 } from "@serial/cpp-bindings-linux/bin";

// Select this with the architecture API provided by your runtime.
const binary = x86_64;

// Decode `binary.data`, write it to `binary.filename`, and load it using your
// runtime's filesystem and native FFI APIs. `binary.ffi` describes the C API
// and its structs for generating or configuring a runtime-specific adapter.
```

Non-JavaScript consumers can download the same architecture-specific `.so` and
`.ffi.json` files directly from the
[GitHub releases](https://github.com/Serial-IO/cpp-bindings-linux/releases).

> [!NOTE]
> For a more in depth guide, check out the
> [Wiki](https://github.com/Serial-IO/cpp-bindings-linux/wiki) section on how to
> use the C++ bindings for Linux.
