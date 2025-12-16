# C++ Bindings Linux

This package ships the native Linux shared library payload as a **JSON/base64 blob**.

## Usage

Import the JSON and write the `.so` to disk (consumer project example):

```ts
import blob from "@serial/cpp-bindings-linux/x84_64" with {type: 'json'};

const bytes = new TextEncoder().encode(atob(blob.data));

const tempFilePath = Deno.makeTempFileSync();
Deno.writeFileSync(tempFilePath, bytes, { mode: 0o755 });

// Now you can open the binary using for example `Deno.dlopen`
