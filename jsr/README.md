# @serial/cpp-bindings-linux

JSR package that ships the native Linux shared library
(`libcpp_bindings_linux.so`) as a **JSON/base64 blob** and reconstructs it
on-demand, because JSR currently doesn't handle binaries as first-class
artifacts.

## Usage

```ts
import { createErrorCallback, loadSerialLib } from "@serial/cpp-bindings-linux";

const { pointer: errPtr, close: closeErr } = createErrorCallback(
    (code, msg) => {
        console.error("native error", code, msg);
    },
);

const lib = await loadSerialLib();
// lib.symbols.serialOpen(...) etc.

// cleanup
closeErr();
lib.close();
```

## Permissions

- `--allow-ffi`
- `--allow-read`
- `--allow-write` (only needed if you use the embedded binary extraction path)
