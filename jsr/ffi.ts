export const symbols = {
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
};

export type LoadedLibrary = Deno.DynamicLibrary<typeof symbols>;
export type SerialLib = LoadedLibrary["symbols"];

export type ErrorCallback = (code: number, message: string) => void;

/**
 * Creates a C-callable callback pointer compatible with `ErrorCallbackT` in the .so
 * (signature: `void (*)(int code, const char* message)`).
 *
 * Remember to `close()` it when you're done.
 */
export function createErrorCallback(cb: ErrorCallback): {
    pointer: Deno.PointerValue;
    close: () => void;
} {
    const callback = new Deno.UnsafeCallback(
        { parameters: ["i32", "pointer"], result: "void" } as const,
        (code: number, messagePtr: Deno.PointerValue) => {
            let message = "";
            try {
                if (messagePtr) {
                    message = new Deno.UnsafePointerView(messagePtr)
                        .getCString();
                }
            } catch {
                // best-effort: ignore malformed pointers
            }
            cb(code, message);
        },
    );

    return {
        pointer: callback.pointer,
        close: () => callback.close(),
    };
}
