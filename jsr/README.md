# @serial/cpp-bindings-linux

JSR package that ships the native Linux shared library payload as a
**JSON/base64 blob** because JSR currently doesn't handle binaries as
first-class artifacts.

## Usage

Import the JSON and write the `.so` to disk (consumer project example):

```ts
import blob from "@serial/cpp-bindings-linux/x84_64" with {
    type: "json",
};

// decode base64 -> bytes
const bin = atob(blob.data);
const bytes = new Uint8Array(bin.length);
for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);

// write library file
const outPath = "./libcpp_bindings_linux.so";
await Deno.writeFile(outPath, bytes, { mode: 0o755 });

// optional: verify sha256 if present
if (blob.sha256) {
    const digest = new Uint8Array(await crypto.subtle.digest("SHA-256", bytes));
    const hex = Array.from(digest, (b) => b.toString(16).padStart(2, "0")).join(
        "",
    );
    if (hex !== blob.sha256) {
        throw new Error(`sha256 mismatch: ${hex} != ${blob.sha256}`);
    }
}

// ... now your other project can dlopen(outPath) / FFI it as needed.
```

## Permissions

- `--allow-read`
- `--allow-write`

## License

This package is licensed under **LGPL-3.0-only** (see the repository root
`LICENSE`).
