/**
 * Minimal FFI bindings for the cpp-bindings-linux shared library
 * used by the Deno integration tests.
 */

export type LoadedLibrary = Deno.DynamicLibrary<typeof symbols>;
export type SerialLib = LoadedLibrary["symbols"];

const symbols = {
    serialOpen: {
        parameters: ["pointer", "i32", "i32", "i32", "i32", "pointer"] as const,
        result: "i64" as const,
    },
    serialClose: {
        parameters: ["i64", "pointer"] as const,
        result: "i32" as const,
    },
    serialRead: {
        parameters: ["i64", "pointer", "i32", "i32", "i32", "pointer"] as const,
        result: "i32" as const,
    },
    serialWrite: {
        parameters: ["i64", "pointer", "i32", "i32", "i32", "pointer"] as const,
        result: "i32" as const,
    },
    serialAbortRead: {
        parameters: ["i64", "pointer"] as const,
        result: "i32" as const,
    },
    serialAbortWrite: {
        parameters: ["i64", "pointer"] as const,
        result: "i32" as const,
    },
};

/**
 * Load the cpp-bindings-linux shared library
 * @param libraryPath Path to the .so file (defaults to build directory)
 * @returns Object containing the symbols and a close method
 */
export async function loadSerialLib(
    libraryPath?: string,
): Promise<LoadedLibrary> {
    // Ensure this stays an async function for API stability
    await Promise.resolve();

    // Try to find the library in common build locations
    const possiblePaths = [
        libraryPath,
        "../build/libcpp_bindings_linux.so",
        "../build/libcpp_bindings_linux.so.0",
        "../build/libcpp_bindings_linux.so.0.0.0",
        "../build/cpp_bindings_linux/libcpp_bindings_linux.so",
        "./libcpp_bindings_linux.so",
        "/usr/local/lib/libcpp_bindings_linux.so",
    ].filter((p): p is string => p !== undefined);

    let lib: LoadedLibrary | null = null;
    let lastError: Error | null = null;

    for (const path of possiblePaths) {
        try {
            const loaded = Deno.dlopen(path, symbols) as LoadedLibrary;
            lib = loaded;
            break;
        } catch (error) {
            lastError = error as Error;
            continue;
        }
    }

    if (!lib) {
        throw new Error(
            `Failed to load cpp-bindings-linux library. Tried paths: ${
                possiblePaths.join(", ")
            }. Last error: ${lastError?.message}`,
        );
    }

    return lib;
}

// Note: helpers for error callbacks and CString handling were removed
// here on purpose, as they are no longer needed by the minimal tests.
