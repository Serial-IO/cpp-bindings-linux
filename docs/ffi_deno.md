# Using cpp_unix_bindings from Deno

Deno ≥ 1.42 ships a first-class **Foreign-Function Interface (FFI)** that lets
JavaScript / TypeScript call native C APIs.  This guide explains the few
patterns you need for smooth interop.

---

## 1. Loading the shared library

```ts
const dylib = Deno.dlopen(
  "./build/libcpp_unix_bindings.so",
  {
    serialOpen:  { parameters: ["pointer", "i32", "i32", "i32", "i32"], result: "pointer" },
    serialClose: { parameters: ["pointer"],                result: "void" },
    // … add further symbols on demand …
  } as const,
);
```

Only the symbols listed in the second argument are made visible to JavaScript.
Always call `dylib.close()` once you are finished.

---

## 2. Passing strings (null-terminated UTF-8)

```ts
const encoder = new TextEncoder();

function cString(str: string): Uint8Array {
  const bytes = encoder.encode(str);
  const buf = new Uint8Array(bytes.length + 1);
  buf.set(bytes);
  buf[bytes.length] = 0; // NUL terminator
  return buf;
}

function ptr(view: Uint8Array): Deno.UnsafePointer {
  return Deno.UnsafePointer.of(view) as Deno.UnsafePointer;
}
```

Usage:

```ts
const handle = dylib.symbols.serialOpen(ptr(cString("/dev/ttyUSB0")), 115200, 8, 0, 0);
```

---

## 3. Working with buffers

```ts
const writeBuf = encoder.encode("Hello\n");
const bytesWritten = dylib.symbols.serialWrite(handle, ptr(writeBuf), writeBuf.length, 100, 1);

const readBuf = new Uint8Array(128);
const n = dylib.symbols.serialRead(handle, ptr(readBuf), readBuf.length, 500, 1);
console.log(new TextDecoder().decode(readBuf.subarray(0, n)));
```

---

## 4. Callbacks

Three optional callbacks can be registered:

* `serialOnError(code: i32, message: *char)`
* `serialOnRead(bytes: i32)`
* `serialOnWrite(bytes: i32)`

```ts
const errorCb = new Deno.UnsafeCallback({
  parameters: ["i32", "pointer"],
  result: "void",
} as const, (code, msgPtr) => {
  const msg = Deno.UnsafePointerView.getCString(msgPtr);
  console.error(`[C-API error] (${code}): ${msg}`);
});

dylib.symbols.serialOnError(errorCb.pointer);
```

**Remember to `close()` the callback** to avoid leaking the function pointer:

```ts
errorCb.close();
```

---

## 5. Cleanup order

```ts
dylib.symbols.serialClose(handle);
// Close callbacks first …
errorCb.close();
// … then the dynamic library

dylib.close();
```

---

Refer to the runnable scripts in `examples/` for end-to-end examples. 
