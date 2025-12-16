/**
 * Minimal Deno integration tests for cpp-bindings-linux:
 * - verify that the shared library can be loaded
 * - verify that it can be cleanly unloaded again
 */

import { assertExists } from "@std/assert";
import { type LoadedLibrary, loadSerialLib, type SerialLib } from "./ffi_bindings.ts";

let lib: SerialLib | null = null;
let loadedLib: LoadedLibrary | null = null;

Deno.test({
    name: "Load cpp-bindings-linux library",
    async fn() {
        // Small async boundary to keep Deno's async test happy
        loadedLib = await loadSerialLib();
        assertExists(loadedLib, "Failed to load cpp-bindings-linux library");
        lib = loadedLib.symbols;

        // Sanity check: verify that the expected symbols exist
        assertExists(lib.serialOpen);
        assertExists(lib.serialClose);
        assertExists(lib.serialRead);
        assertExists(lib.serialWrite);

        console.log("cpp-bindings-linux library loaded and symbols resolved");
    },
    sanitizeResources: false,
    sanitizeOps: false,
});

Deno.test({
    name: "Unload cpp-bindings-linux library",
    async fn() {
        // Simulate async cleanup boundary
        await Promise.resolve();
        if (!loadedLib) return;

        loadedLib.close();
        loadedLib = null;
        lib = null;
        console.log("cpp-bindings-linux library unloaded successfully");
    },
    sanitizeResources: false,
    sanitizeOps: false,
});
