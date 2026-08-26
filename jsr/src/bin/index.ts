/**
 * Module that provides exports for serialized binary files.
 * 
 * @example
 * Import the corresponding binary and write the file to disk.
 * 
 * ```ts
 * import {aarch64, x86_64} from '@serial/cpp-bindings-linux/bin'
 * 
 * const binary = Deno.build.arch === "aarch64" ? aarch64 : x86_64
 * Deno.writeFileSync(`./${binary.filename}`, Uint8Array.fromBase64(binary.data))
 * ```
 * @module
 */

import aarch64 from '../../bin/aarch64.json' with { type: "json" }
import x86_64 from '../../bin/x86_64.json' with { type: "json" }

export {aarch64, x86_64}
