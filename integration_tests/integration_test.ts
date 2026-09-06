/**
 * Minimal Deno integration tests for cpp-bindings-linux:
 * - verify that the shared library can be loaded
 * - verify that it can be cleanly unloaded again
 */

import { assertEquals, assertExists } from "@std/assert";
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
    name: "cpp-core v3 configuration ABI",
    fn() {
        assertExists(lib);
        const config = new Int32Array([9600, 8, 0, 0, 0]);
        const timeout = new Int32Array([10, 1]);
        const invalidTimeout = new Int32Array([-1, 1]);
        const port = new TextEncoder().encode("/dev/ttyNONEXISTENT99999\0");
        const buffer = new Uint8Array(4);
        const pointer = Deno.UnsafePointer.of;

        assertEquals(Number(lib.serialOpen(pointer(port), pointer(config), null)), -200);
        assertEquals(Number(lib.serialOpen(pointer(port), null, null)), -405);
        assertEquals(lib.serialRead(-1n, pointer(buffer), 4, pointer(timeout), null), -201);
        assertEquals(lib.serialWrite(-1n, pointer(buffer), 4, pointer(timeout), null), -201);
        assertEquals(lib.serialRead(-1n, pointer(buffer), 4, null, null), -105);
        assertEquals(lib.serialWrite(-1n, pointer(buffer), 4, pointer(invalidTimeout), null), -105);
        assertEquals(
            lib.serialReadUntilSequence(-1n, pointer(buffer), 4, pointer(timeout), pointer(buffer), 0, null),
            -304,
        );
        assertEquals(lib.serialWaitForDrain(-1n, null), -201);
        assertEquals(lib.serialSetEventCallback(null, null), 0);
        lib.meta(null);
    },
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
