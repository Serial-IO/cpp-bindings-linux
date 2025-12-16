import {
    createErrorCallback,
    type LoadedLibrary,
    type SerialLib,
    symbols,
} from "./ffi.ts";

type EmbeddedBinary = {
    target: string;
    filename: string;
    encoding: "base64";
    sha256?: string;
    data: string;
};

let extractedLibraryPath: string | null = null;

function base64ToBytes(base64: string): Uint8Array {
    const bin = atob(base64);
    const out = new Uint8Array(bin.length);
    for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
    return out;
}

async function loadEmbeddedBinary(): Promise<EmbeddedBinary> {
    const os = Deno.build.os;
    const arch = Deno.build.arch;
    const target = `${os}-${arch}`;

    if (target !== "linux-x86_64") {
        throw new Error(
            `@serial/cpp-bindings-linux only ships an embedded binary for linux-x86_64 right now (got ${target}).`,
        );
    }

    // Keep this as a dynamic import so local dev builds don't require a real binary JSON.
    const mod = await import("./binaries/linux-x86_64.json", {
        with: { type: "json" },
    });
    return mod.default as EmbeddedBinary;
}

export type EnsureLibraryOptions = {
    /**
     * Directory to write the extracted .so into.
     * Defaults to a temporary directory.
     */
    dir?: string;
    /**
     * If true, re-write the file even if we already extracted it in this process.
     */
    force?: boolean;
};

/**
 * Extract the embedded `.so` (stored as JSON/base64) to a real file on disk and return its path.
 *
 * Required permissions:
 * - `--allow-read` (to read the embedded JSON in the package)
 * - `--allow-write` (to write the `.so` to disk)
 */
export async function ensureSharedLibraryFile(
    options: EnsureLibraryOptions = {},
): Promise<string> {
    if (extractedLibraryPath && !options.force) return extractedLibraryPath;

    const embedded = await loadEmbeddedBinary();
    if (!embedded.data) {
        throw new Error(
            "Embedded binary JSON is empty. If you're running from source, generate it via jsr/scripts/embed_so.ts.",
        );
    }

    const dir = options.dir ??
        await Deno.makeTempDir({ prefix: "serial-cpp-bindings-linux-" });
    const path = `${dir}/${embedded.filename}`;
    const bytes = base64ToBytes(embedded.data);

    await Deno.writeFile(path, bytes, { mode: 0o755 });
    extractedLibraryPath = path;
    return path;
}

export type LoadSerialLibOptions = {
    /**
     * If provided, skips extraction and loads the .so from this path.
     */
    libraryPath?: string;
    /**
     * Directory to write the extracted .so into (if `libraryPath` is not provided).
     */
    extractDir?: string;
    /**
     * Force re-extract (useful if you manage the directory yourself).
     */
    forceExtract?: boolean;
};

/**
 * Load the native library via `Deno.dlopen`.
 *
 * Required permissions:
 * - `--allow-ffi`
 * - `--allow-read` (to load the .so)
 * - `--allow-write` (only if extracting the embedded binary)
 */
export async function loadSerialLib(
    options: LoadSerialLibOptions = {},
): Promise<LoadedLibrary> {
    await Promise.resolve(); // keep async for API stability

    const path = options.libraryPath ??
        await ensureSharedLibraryFile({
            dir: options.extractDir,
            force: options.forceExtract,
        });

    return Deno.dlopen(path, symbols) as LoadedLibrary;
}

export { createErrorCallback, symbols };
export type { LoadedLibrary, SerialLib };
