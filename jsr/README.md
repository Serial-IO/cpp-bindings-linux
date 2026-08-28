# C++ Bindings Linux

[![Build Binary](https://github.com/Serial-IO/cpp-bindings-linux/actions/workflows/build_binary.yml/badge.svg)](https://github.com/Serial-IO/cpp-bindings-linux/actions/workflows/build_binary.yml)
[![JSR](https://jsr.io/badges/@serial/cpp-bindings-linux)](https://jsr.io/@serial/cpp-bindings-linux)

Binaries are provided as a
[package on JSR](https://jsr.io/@serial/cpp-bindings-linux). They are serialized
as a base64 string inside the JSON file.

The package contains portable binaries for `x86_64-linux-gnu` and
`aarch64-linux-gnu`, both requiring glibc 2.28 or newer. The x86-64 artifact
uses the generic x86-64 baseline.

It also includes cpp-core FFI API metadata generated with
[ASTrein](https://github.com/Katze719/ASTrein) at `bin/x86_64/ffi.json` and
`bin/aarch64/ffi.json`. It describes the exported C symbols, parameter and
return types, callbacks, default values, and API documentation used by
downstream FFI adapter generators.

This package is primarily intended as a dependency for
[`@serial/serial`](https://jsr.io/@serial/serial). However, it can also be used
independently.

## Usage

Import the JSON and write the binary data to disk. Each architecture export also
contains its matching FFI metadata:

```ts
import { aarch64, x86_64 } from "@serial/cpp-bindings-linux/bin";

const binary = Deno.build.arch === "aarch64" ? aarch64 : x86_64;
Deno.writeFileSync(`./${binary.filename}`, Uint8Array.fromBase64(binary.data));

// The matching FFI metadata is available as `binary.ffi`.
// Now you can open the binary using for example `Deno.dlopen`...
```

> [!NOTE]
> For a more in depth guide, check out the
> [Wiki](https://github.com/Serial-IO/cpp-bindings-linux/wiki) section on how to
> use the C++ bindings for Linux.
