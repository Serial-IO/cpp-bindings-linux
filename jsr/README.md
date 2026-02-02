# C++ Bindings Linux

[![Build Binary](https://github.com/Serial-IO/cpp-bindings-linux/actions/workflows/build_binary.yml/badge.svg)](https://github.com/Serial-IO/cpp-bindings-linux/actions/workflows/build_binary.yml)
[![JSR](https://jsr.io/badges/@serial/cpp-bindings-linux)](https://jsr.io/@serial/cpp-bindings-linux)

Binaries are provided as a [package on JSR](https://jsr.io/@serial/cpp-bindings-linux). They are serialized as a base64 string inside the JSON file.

This package is primarily intended as a dependency for [`@serial/serial`](https://jsr.io/@serial/serial). However, it can also be used independently.


## Usage

Import the JSON and write the binary data to disk:

```ts
import {x86_64} from '@serial/cpp-bindings-linux/bin';

Deno.writeFileSync(`./${x86_64.filename}`, Uint8Array.fromBase64(x86_64.data));

// Now you can open the binary using for example `Deno.dlopen`...
```

> [!NOTE]
> For a more in depth guide, check out the [Wiki](https://github.com/Serial-IO/cpp-bindings-linux/wiki) section on how to use the C++ bindings for Linux.
