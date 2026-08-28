/**
 * Module that provides serialized binaries and FFI metadata.
 *
 * @example
 * Import the corresponding binary and write the file to disk.
 *
 * ```ts
 * import { aarch64, x86_64 } from "@serial/cpp-bindings-linux/bin";
 *
 * const binary = Deno.build.arch === "aarch64" ? aarch64 : x86_64;
 *
 * Deno.writeFileSync(
 *   `./${binary.filename}`,
 *   Uint8Array.fromBase64(binary.data),
 * );
 *
 * // The matching FFI metadata is available as `binary.ffi`.
 * ```
 * @module
 */

import aarch64Library from "../../bin/aarch64/library.json" with {
  type: "json",
};
import aarch64ffi from "../../bin/aarch64/ffi.json" with { type: "json" };
import x86_64Library from "../../bin/x86_64/library.json" with {
  type: "json",
};
import x86_64ffi from "../../bin/x86_64/ffi.json" with { type: "json" };

const aarch64 = {
  ...aarch64Library,
  ffi: aarch64ffi,
};

const x86_64 = {
  ...x86_64Library,
  ffi: x86_64ffi,
};

export { aarch64, x86_64 };
