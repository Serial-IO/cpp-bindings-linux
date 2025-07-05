// @ts-nocheck
// deno run --allow-ffi --allow-read examples/serial_callbacks.ts
//
// Demonstrates the optional C-API callbacks (error, read, write).
// The script registers three callbacks that print diagnostics and then
// performs a simple line-oriented echo test.
//
// CLI flags:
//   --lib  <path>   Path to the shared library (default ./build/libcpp_unix_bindings.so)
//   --port <path>   Serial device path     (default /dev/ttyUSB0)

interface CliOptions {
    lib: string;
    port?: string;
}

function parseArgs(): CliOptions {
    const opts: CliOptions = { lib: "./build/libcpp_unix_bindings.so" };

    for (let i = 0; i < Deno.args.length; ++i) {
        const arg = Deno.args[i];
        if (arg === "--lib" && i + 1 < Deno.args.length) {
            opts.lib = Deno.args[++i];
        } else if (arg === "--port" && i + 1 < Deno.args.length) {
            opts.port = Deno.args[++i];
        } else {
            console.warn(`Unknown argument '${arg}' ignored.`);
        }
    }
    return opts;
}

// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------
const encoder = new TextEncoder();
const decoder = new TextDecoder();

function cString(str: string): Uint8Array {
    const bytes = encoder.encode(str);
    const buf = new Uint8Array(bytes.length + 1);
    buf.set(bytes, 0);
    buf[bytes.length] = 0;
    return buf;
}

function ptr(view: Uint8Array): Deno.UnsafePointer {
    return Deno.UnsafePointer.of(view) as Deno.UnsafePointer;
}

// -----------------------------------------------------------------------------
// Load library & register callbacks
// -----------------------------------------------------------------------------
const { lib, port: cliPort } = parseArgs();

const dylib = Deno.dlopen(
    lib,
    {
        serialOpen: { parameters: ["pointer", "i32", "i32", "i32", "i32"], result: "pointer" },
        serialClose: { parameters: ["pointer"], result: "void" },
        serialWriteLine: { parameters: ["pointer", "pointer", "i32", "i32"], result: "i32" },
        serialReadLine: { parameters: ["pointer", "pointer", "i32", "i32"], result: "i32" },
        serialOnError: { parameters: ["pointer"], result: "void" },
        serialOnRead: { parameters: ["pointer"], result: "void" },
        serialOnWrite: { parameters: ["pointer"], result: "void" },
        serialDrain: { parameters: ["pointer"], result: "i32" },
    } as const,
);

const errorCb = new Deno.UnsafeCallback({
    parameters: ["i32", "pointer"],
    result: "void",
} as const, (code, msgPtr) => {
    const msg = Deno.UnsafePointerView.getCString(msgPtr);
    console.error(`[C-ERROR] (${code}): ${msg}`);
});

const readCb = new Deno.UnsafeCallback({
    parameters: ["i32"],
    result: "void",
} as const, (bytes) => {
    console.log(`[C-READ]  ${bytes} byte(s) received`);
});

const writeCb = new Deno.UnsafeCallback({
    parameters: ["i32"],
    result: "void",
} as const, (bytes) => {
    console.log(`[C-WRITE] ${bytes} byte(s) transmitted`);
});

dylib.symbols.serialOnError(errorCb.pointer);
dylib.symbols.serialOnRead(readCb.pointer);
dylib.symbols.serialOnWrite(writeCb.pointer);

// -----------------------------------------------------------------------------
// Open port
// -----------------------------------------------------------------------------
const portPath = cliPort ?? "/dev/ttyUSB0";
console.log(`Opening port: ${portPath}`);

let handle: Deno.PointerValue | null = dylib.symbols.serialOpen(ptr(cString(portPath)), 115200, 8, 0, 0);
if (handle === null) {
    console.error("Failed to open port.");
    cleanup();
    Deno.exit(1);
}

// Give MCU time to reset (common on Arduino)
await new Promise((r) => setTimeout(r, 2000));

// -----------------------------------------------------------------------------
// Echo interaction
// -----------------------------------------------------------------------------
const message = `Callback test ${Date.now()}`;
const msgBuf = encoder.encode(message);

console.log(`Sending: '${message}'`);
const written = dylib.symbols.serialWriteLine(handle, ptr(msgBuf), msgBuf.length, 100);
if (written !== msgBuf.length + 1) {
    console.error("serialWriteLine wrote unexpected byte count");
}

const rxBuf = new Uint8Array(256);
const n = dylib.symbols.serialReadLine(handle, ptr(rxBuf), rxBuf.length, 1000);
const response = decoder.decode(rxBuf.subarray(0, n));

console.log(`Received (${n} bytes): '${response}'`);

if (response === message) {
    console.log("Echo OK ✅");
} else {
    console.warn("Echo mismatch ⚠");
}

// Wait until TX queue is empty then close
if (dylib.symbols.serialDrain(handle) === 1) {
    console.log("Transmit buffer drained.");
}

cleanup();

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
function cleanup() {
    if (handle !== null) {
        dylib.symbols.serialClose(handle);
        handle = null;
    }
    errorCb.close();
    readCb.close();
    writeCb.close();
    dylib.close();
} 
