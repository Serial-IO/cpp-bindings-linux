/**
 * Integration tests for cpp-bindings-linux using Deno FFI
 */

import { assert, assertEquals, assertExists, assertStringIncludes } from "@std/assert";
import {
    createErrorCallback,
    type LoadedLibrary,
    loadSerialLib,
    type SerialLib,
    stringToCString,
} from "./ffi_bindings.ts";

const StatusCodes = {
    kSuccess: 0,
    kCloseHandleError: -1,
    kInvalidHandleError: -2,
    kReadError: -3,
    kWriteError: -4,
    kGetStateError: -5,
    kSetStateError: -6,
    kSetTimeoutError: -7,
    kBufferError: -8,
    kNotFoundError: -9,
    kClearBufferInError: -10,
    kClearBufferOutError: -11,
    kAbortReadError: -12,
    kAbortWriteError: -13,
} as const;

let lib: SerialLib | null = null;
let loadedLib: LoadedLibrary | null = null;

Deno.test({
    name: "Load library",
    async fn() {
        loadedLib = await loadSerialLib();
        assertExists(loadedLib, "Failed to load library");
        lib = loadedLib.symbols;
        console.log("Library loaded successfully");
    },
    sanitizeResources: false,
    sanitizeOps: false,
});

Deno.test({
    name: "Invalid handle - Close",
    async fn() {
        assertExists(lib, "Library not loaded");

        const result = lib.serialClose(BigInt(-1), null);
        assertEquals(result, StatusCodes.kSuccess);
        console.log("Invalid handle close handled correctly");
    },
});

Deno.test({
    name: "Invalid handle - Read",
    async fn() {
        assertExists(lib, "Library not loaded");

        const buffer = new Uint8Array(256);
        const bufferPtr = Deno.UnsafePointer.of(buffer);
        assertExists(bufferPtr, "Failed to create buffer pointer");

        const result = lib.serialRead(
            BigInt(-1),
            bufferPtr,
            buffer.length,
            1000,
            1,
            null,
        );

        assertEquals(result, StatusCodes.kInvalidHandleError);
        console.log("Invalid handle read handled correctly");
    },
});

Deno.test({
    name: "Invalid handle - Write",
    async fn() {
        assertExists(lib, "Library not loaded");

        const message = "test";
        const messagePtr = stringToCString(message);

        const result = lib.serialWrite(
            BigInt(-1),
            messagePtr,
            message.length,
            1000,
            1,
            null,
        );

        assertEquals(result, StatusCodes.kInvalidHandleError);
        console.log("Invalid handle write handled correctly");
    },
});

Deno.test({
    name: "Error callback - Invalid port",
    async fn() {
        assertExists(lib, "Library not loaded");

        let callbackCalled = false;
        let receivedStatusCode = -1;
        let receivedMessage = "";

        const errorCallback = createErrorCallback((statusCode, message) => {
            callbackCalled = true;
            receivedStatusCode = statusCode;
            receivedMessage = message;
        });

        try {
            const result = lib.serialOpen(
                null,
                115200,
                8,
                0,
                1,
                errorCallback.pointer,
            );

            assert(result < 0, `Expected error code, got ${result}`);

            await new Promise((resolve) => setTimeout(resolve, 100));

            assert(callbackCalled, "Error callback was not called");

            assertEquals(receivedStatusCode, StatusCodes.kNotFoundError);

            assertStringIncludes(
                receivedMessage,
                "nullptr",
                `Expected error message about nullptr, got: ${receivedMessage}`,
            );

            console.log("Error callback works correctly");
        } finally {
            errorCallback.close();
        }
    },
});

Deno.test({
    name: "Invalid parameters - Baudrate",
    async fn() {
        assertExists(lib, "Library not loaded");

        const port = "/dev/ttyUSB0";
        const portPtr = stringToCString(port);

        let callbackCalled = false;
        let receivedStatusCode = -1;
        let receivedMessage = "";
        const errorCallback = createErrorCallback((statusCode, message) => {
            callbackCalled = true;
            receivedStatusCode = statusCode;
            receivedMessage = message;
        });

        try {
            const result = lib.serialOpen(
                portPtr,
                100,
                8,
                0,
                1,
                errorCallback.pointer,
            );

            await new Promise((resolve) => setTimeout(resolve, 100));

            assert(result < 0, `Expected error code, got ${result}`);
            assert(callbackCalled, "Error callback was not called");

            assertEquals(receivedStatusCode, StatusCodes.kSetStateError);
            assertStringIncludes(
                receivedMessage,
                "baudrate",
                `Expected baudrate error, got: ${receivedMessage}`,
            );

            console.log("Invalid baudrate handled correctly");
        } finally {
            errorCallback.close();
        }
    },
});

Deno.test({
    name: "Invalid parameters - Data bits",
    async fn() {
        assertExists(lib, "Library not loaded");

        const port = "/dev/ttyUSB0";
        const portPtr = stringToCString(port);

        let callbackCalled = false;
        let receivedStatusCode = -1;
        const errorCallback = createErrorCallback((statusCode, message) => {
            callbackCalled = true;
            receivedStatusCode = statusCode;
        });

        try {
            const result = lib.serialOpen(
                portPtr,
                115200,
                3,
                0,
                1,
                errorCallback.pointer,
            );

            await new Promise((resolve) => setTimeout(resolve, 100));

            assert(result < 0, `Expected error code, got ${result}`);
            assert(callbackCalled, "Error callback was not called");

            assertEquals(receivedStatusCode, StatusCodes.kSetStateError);

            console.log("Invalid data bits handled correctly");
        } finally {
            errorCallback.close();
        }
    },
});

Deno.test({
    name: "Real serial port - Open/Close",
    async fn() {
        assertExists(lib, "Library not loaded");

        const port = "/dev/ttyUSB0";
        const portPtr = stringToCString(port);

        const handle = lib.serialOpen(
            portPtr,
            115200,
            8,
            0,
            1,
            null,
        );

        if (handle <= 0) {
            console.log("Skipping test: /dev/ttyUSB0 not available");
            return;
        }

        try {
            await new Promise((resolve) => setTimeout(resolve, 2000));

            const closeResult = lib.serialClose(handle, null);
            assertEquals(closeResult, StatusCodes.kSuccess);

            console.log("Real serial port open/close works");
        } catch (error) {
            lib.serialClose(handle, null);
            throw error;
        }
    },
    ignore: Deno.env.get("SKIP_HARDWARE_TESTS") === "1",
});

Deno.test({
    name: "Real serial port - Write/Read (if Arduino echo available)",
    async fn() {
        assertExists(lib, "Library not loaded");

        const port = "/dev/ttyUSB0";
        const portPtr = stringToCString(port);

        const handle = lib.serialOpen(
            portPtr,
            115200,
            8,
            0,
            1,
            null,
        );

        if (handle <= 0) {
            console.log("Skipping test: /dev/ttyUSB0 not available");
            return;
        }

        try {
            await new Promise((resolve) => setTimeout(resolve, 2000));

            const testMessage = "Hello from Deno!\n";
            const encoder = new TextEncoder();
            const messageBytes = encoder.encode(testMessage);
            const messageBuffer = new Uint8Array(messageBytes.length);
            messageBuffer.set(messageBytes);
            const messagePtr = Deno.UnsafePointer.of(messageBuffer);
            assertExists(messagePtr, "Failed to create message pointer");

            const written = lib.serialWrite(
                handle,
                messagePtr,
                messageBytes.length,
                1000,
                1,
                null,
            );

            assertEquals(written, messageBytes.length);

            await new Promise((resolve) => setTimeout(resolve, 500));

            const readBuffer = new Uint8Array(256);
            const readBufferPtr = Deno.UnsafePointer.of(readBuffer);
            if (readBufferPtr === null) {
                throw new Error("Failed to create read buffer pointer");
            }

            const readBytes = lib.serialRead(
                handle,
                readBufferPtr,
                readBuffer.length - 1,
                2000,
                1,
                null,
            );

            if (readBytes > 0) {
                const receivedData = new Uint8Array(readBytes);
                receivedData.set(readBuffer.subarray(0, readBytes));
                const decoder = new TextDecoder();
                const received = decoder.decode(receivedData);
                console.log(`Received echo: ${received.trim()}`);
            } else {
                console.log("No data received (device might not echo)");
            }
        } finally {
            lib.serialClose(handle, null);
        }
    },
    ignore: Deno.env.get("SKIP_HARDWARE_TESTS") === "1",
});

Deno.test({
    name: "Unload library",
    async fn() {
        if (loadedLib) {
            loadedLib.close();
            loadedLib = null;
            lib = null;
            console.log("Library unloaded successfully");
        }
    },
    sanitizeResources: false,
    sanitizeOps: false,
});
