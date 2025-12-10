/**
 * FFI bindings for cpp-bindings-linux shared library
 */

// Error callback type: (status_code: number, message: string) => void
export type ErrorCallback = (statusCode: number, message: string) => void;

// FFI symbol definitions
export type SerialLib = {
  readonly serialOpen: (
    port: Deno.PointerValue,
    baudrate: number,
    dataBits: number,
    parity: number,
    stopBits: number,
    errorCallback: Deno.PointerValue | null,
  ) => bigint;

  readonly serialClose: (
    handle: bigint,
    errorCallback: Deno.PointerValue | null,
  ) => number;

  readonly serialRead: (
    handle: bigint,
    buffer: Deno.PointerValue,
    bufferSize: number,
    timeoutMs: number,
    multiplier: number,
    errorCallback: Deno.PointerValue | null,
  ) => number;

  readonly serialWrite: (
    handle: bigint,
    buffer: Deno.PointerValue,
    bufferSize: number,
    timeoutMs: number,
    multiplier: number,
    errorCallback: Deno.PointerValue | null,
  ) => number;
};

// Library object type
export type LoadedLibrary = {
  symbols: SerialLib;
  close: () => void;
};

/**
 * Load the cpp-bindings-linux shared library
 * @param libraryPath Path to the .so file (defaults to build directory)
 * @returns Object containing the symbols and a close method
 */
export async function loadSerialLib(
  libraryPath?: string,
): Promise<LoadedLibrary> {
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

  let lib: { symbols: SerialLib; close: () => void } | null = null;
  let lastError: Error | null = null;

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
  };

  for (const path of possiblePaths) {
    try {
      const loaded = Deno.dlopen(path, symbols) as {
        symbols: SerialLib;
        close: () => void;
      };
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

/**
 * Create an error callback function that can be passed to FFI
 */
export function createErrorCallback(
  callback: ErrorCallback,
): Deno.UnsafeCallback {
  const definition = {
    parameters: ["i32", "pointer"] as const,
    result: "void" as const,
  };

  // @ts-expect-error - UnsafeCallback callback signature is complex in Deno 2.6.0
  return new Deno.UnsafeCallback(
    definition,
    (
      ...args: (number | bigint | boolean | Deno.PointerValue | Uint8Array)[]
    ): void => {
      const statusCode = args[0] as number;
      const messagePtr = args[1] as Deno.PointerValue;

      if (messagePtr === null) {
        callback(statusCode, "Unknown error");
        return;
      }

      // Read the C string from the pointer
      const message = Deno.UnsafePointerView.getCString(messagePtr);
      callback(statusCode, message);
    },
  );
}

/**
 * Helper to convert a string to a C string pointer
 * Note: The returned pointer is only valid as long as the buffer is in scope.
 * For longer-lived strings, use a persistent buffer.
 */
export function stringToCString(str: string): Deno.PointerValue {
  const encoder = new TextEncoder();
  const encoded = encoder.encode(str + "\0");
  // Create a buffer that will persist
  const buffer = new Uint8Array(encoded.length);
  buffer.set(encoded);
  const ptr = Deno.UnsafePointer.of(buffer);
  if (ptr === null) {
    throw new Error("Failed to create pointer from string");
  }
  return ptr;
}

/**
 * Helper to read a C string from a pointer
 */
export function cStringToString(ptr: Deno.PointerValue): string {
  if (ptr === null) {
    return "";
  }
  return Deno.UnsafePointerView.getCString(ptr);
}
