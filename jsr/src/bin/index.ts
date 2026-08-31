/**
 * Module that provides serialized binaries and FFI metadata.
 *
 * @example
 * Import the corresponding binary and write the file to disk.
 *
 * ```ts
 * import { aarch64, x86_64 } from "@serial/cpp-bindings-linux/bin";
 *
 * // Select this with the architecture API provided by your runtime.
 * const binary = x86_64;
 *
 * // Decode `binary.data`, write it to `binary.filename`, and load it using
 * // your runtime's filesystem and native FFI APIs. The matching C API
 * // metadata, including struct definitions, is available as `binary.ffi`.
 * ```
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
