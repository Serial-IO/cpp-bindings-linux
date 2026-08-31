/**
 * Module that provides serialized binaries and FFI metadata.
 *
 * @example
 * Usage with Deno
 *
 * ```ts
 * import { aarch64, x86_64 } from "@serial/cpp-bindings-linux/bin";
 *
 * const binary = Deno.build.arch === "aarch64" ? aarch64 : x86_64;
 * const path = `./${binary.filename}`;
 *
 * Deno.writeFileSync(path, Uint8Array.fromBase64(binary.data));
 *
 * const library = Deno.dlopen(path, {
 *   serialOpen: {
 *     parameters: ["pointer", "i32", "i32", "i32", "i32", "pointer"],
 *     result: "i64",
 *   },
 * });
 * library.close();
 * ```
 *
 * @example
 * Usage with Bun
 *
 * ```ts
 * import { dlopen } from "bun:ffi";
 * import { resolve } from "node:path";
 * import { aarch64, x86_64 } from "@serial/cpp-bindings-linux/bin";
 *
 * const binary = process.arch === "arm64"
 *   ? aarch64
 *   : process.arch === "x64"
 *   ? x86_64
 *   : undefined;
 *
 * if (!binary) {
 *   throw new Error(`Unsupported architecture: ${process.arch}`);
 * }
 *
 * const path = resolve(binary.filename);
 * await Bun.write(path, Buffer.from(binary.data, "base64"));
 *
 * const library = dlopen(path, {
 *   serialOpen: {
 *     args: ["ptr", "i32", "i32", "i32", "i32", "ptr"],
 *     returns: "i64",
 *   },
 * });
 * library.close();
 * ```
 *
 * @example
 * Usage with Node.js
 *
 * ```js
 * import { writeFileSync } from "node:fs";
 * import { resolve } from "node:path";
 * import koffi from "koffi";
 * import { aarch64, x86_64 } from "@serial/cpp-bindings-linux/bin";
 *
 * const binary = process.arch === "arm64"
 *   ? aarch64
 *   : process.arch === "x64"
 *   ? x86_64
 *   : undefined;
 *
 * if (!binary) {
 *   throw new Error(`Unsupported architecture: ${process.arch}`);
 * }
 *
 * const path = resolve(binary.filename);
 * writeFileSync(path, Buffer.from(binary.data, "base64"));
 *
 * const library = koffi.load(path);
 * library.func("serialOpen", "int64_t", [
 *   "void *",
 *   "int",
 *   "int",
 *   "int",
 *   "int",
 *   "void *",
 * ]);
 * library.unload();
 * ```
 *
 * The matching C API metadata, including struct definitions, is available as
 * `binary.ffi` in every runtime.
 * @module
 */

import aarch64Library from "../../bin/aarch64/library.json" with { type: "json" };
import aarch64ffi from "../../bin/aarch64/ffi.json" with { type: "json" };
import x86_64Library from "../../bin/x86_64/library.json" with { type: "json" };
import x86_64ffi from "../../bin/x86_64/ffi.json" with { type: "json" };

/**
 * The serialized `aarch64-linux-gnu` shared library and its FFI metadata.
 *
 * The library targets ARMv8-A and requires glibc 2.28 or newer. Decode `data`
 * from base64, write it to `filename`, and load it using the filesystem and
 * native FFI APIs provided by your runtime.
 */
const aarch64 = {
  ...aarch64Library,
  /**
   * ASTrein-generated metadata describing the library's exported C API.
   *
   * It contains symbols, parameter and return types, callbacks, struct
   * definitions, default values, and API documentation for generating
   * runtime-specific FFI adapters.
   */
  ffi: aarch64ffi,
};

/**
 * The serialized `x86_64-linux-gnu` shared library and its FFI metadata.
 *
 * The library targets the generic x86-64 baseline and requires glibc 2.28 or
 * newer. Decode `data` from base64 and write it to `filename` before loading
 * it using the filesystem and native FFI APIs provided by your runtime.
 */
const x86_64 = {
  ...x86_64Library,
  /**
   * ASTrein-generated metadata describing the library's exported C API.
   *
   * It contains symbols, parameter and return types, callbacks, struct
   * definitions, default values, and API documentation for generating
   * runtime-specific FFI adapters.
   */
  ffi: x86_64ffi,
};

export { aarch64, x86_64 };
