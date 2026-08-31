# C++ Bindings Linux

[![Build Binary](https://github.com/Serial-IO/cpp-bindings-linux/actions/workflows/build_binary.yml/badge.svg)](https://github.com/Serial-IO/cpp-bindings-linux/actions/workflows/build_binary.yml)
[![JSR](https://jsr.io/badges/@serial/cpp-bindings-linux)](https://jsr.io/@serial/cpp-bindings-linux)

Binaries are provided as a
[package on JSR](https://jsr.io/@serial/cpp-bindings-linux). They are serialized
as a base64 string inside the JSON file.

This package targets server-side JavaScript runtimes that can write files and
load GNU/Linux shared libraries. Deno can consume it directly from JSR; Bun and
Node.js use JSR's npm compatibility layer. Browser and edge runtimes cannot use
the native library because they do not expose native FFI access.

The package contains portable binaries for `x86_64-linux-gnu` and
`aarch64-linux-gnu`, both requiring glibc 2.28 or newer. The x86-64 artifact
uses the generic x86-64 baseline.

## Binary compatibility

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

## FFI metadata

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
base64-encoded shared library and its matching FFI metadata. The following
examples write the library to disk, load it, and release it again.

### Deno

Deno provides native JSR imports and the built-in `Deno.dlopen` FFI API. Save
this as `example.ts`:

```ts
import { aarch64, x86_64 } from "jsr:@serial/cpp-bindings-linux/bin";

const binary = Deno.build.arch === "aarch64" ? aarch64 : x86_64;
const path = `./${binary.filename}`;

Deno.writeFileSync(path, Uint8Array.fromBase64(binary.data));

const library = Deno.dlopen(path, {
  serialClose: {
    parameters: ["i64", "pointer"],
    result: "i32",
  },
});
library.close();
```

Run it with write and FFI permissions:

```sh
deno run --allow-write --allow-ffi example.ts
```

### Bun

Add the package through JSR's npm compatibility layer:

```sh
bunx jsr add @serial/cpp-bindings-linux
```

Then use Bun's built-in `bun:ffi` and `Bun.write` APIs:

```ts
import { dlopen } from "bun:ffi";
import { resolve } from "node:path";
import { aarch64, x86_64 } from "@serial/cpp-bindings-linux/bin";

const binary = process.arch === "arm64"
  ? aarch64
  : process.arch === "x64"
  ? x86_64
  : undefined;

if (!binary) {
  throw new Error(`Unsupported architecture: ${process.arch}`);
}

const path = resolve(binary.filename);
await Bun.write(path, Buffer.from(binary.data, "base64"));

const library = dlopen(path, {
  serialClose: {
    args: ["i64", "ptr"],
    returns: "i32",
  },
});
library.close();
```

```sh
bun run example.ts
```

> [!WARNING]
> Bun currently marks its built-in
> [`bun:ffi` API](https://bun.sh/docs/runtime/ffi) as experimental.

### Node.js

Node.js does not provide a general-purpose C FFI API. This example uses
[Koffi](https://koffi.dev/), together with JSR's npm compatibility layer:

```sh
npx jsr add @serial/cpp-bindings-linux
npm install koffi
```

Save this as `example.mjs`:

```js
import { writeFileSync } from "node:fs";
import { resolve } from "node:path";
import koffi from "koffi";
import { aarch64, x86_64 } from "@serial/cpp-bindings-linux/bin";

const binary = process.arch === "arm64"
  ? aarch64
  : process.arch === "x64"
  ? x86_64
  : undefined;

if (!binary) {
  throw new Error(`Unsupported architecture: ${process.arch}`);
}

const path = resolve(binary.filename);
writeFileSync(path, Buffer.from(binary.data, "base64"));

const library = koffi.load(path);
library.func("int serialClose(int64_t handle, void *error_callback)");
library.unload();
```

```sh
node example.mjs
```

These examples verify that the native library can be loaded and that its
`serialClose` symbol can be resolved. The matching `binary.ffi` value describes
the complete set of symbols and structs for generating or configuring
runtime-specific bindings.

Non-JavaScript consumers can download the same architecture-specific `.so` and
`.ffi.json` files directly from the
[GitHub releases](https://github.com/Serial-IO/cpp-bindings-linux/releases).

> [!NOTE]
> For a more in depth guide, check out the
> [Wiki](https://github.com/Serial-IO/cpp-bindings-linux/wiki) section on how to
> use the C++ bindings for Linux.
