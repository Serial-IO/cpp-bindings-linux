// Usage:
//   deno run --allow-read --allow-write jsr/scripts/embed_so.ts \
//     ./build/libcpp_bindings_linux.so ./jsr/bin/x84_64.json linux-x86_64
//
// This converts the shared library into a JSON file containing base64 data for publishing to JSR.

function bytesToHex(bytes: Uint8Array): string {
    return Array.from(bytes, (b) => b.toString(16).padStart(2, "0")).join("");
}

function base64FromBytes(bytes: Uint8Array): string {
    // Chunk to avoid call stack limits in String.fromCharCode(...bigArray)
    const chunkSize = 0x8000;
    let binary = "";
    for (let i = 0; i < bytes.length; i += chunkSize) {
        const chunk = bytes.subarray(i, i + chunkSize);
        binary += String.fromCharCode(...chunk);
    }
    return btoa(binary);
}

if (import.meta.main) {
    const [inPath, outPath, target = "linux-x86_64"] = Deno.args;
    if (!inPath || !outPath) {
        console.error(
            "Expected: <input .so path> <output .json path> [target]\nExample: build/libcpp_bindings_linux.so jsr/bin/x84_64.json linux-x86_64",
        );
        Deno.exit(2);
    }

    const bytes = await Deno.readFile(inPath);
    const digest = new Uint8Array(await crypto.subtle.digest("SHA-256", bytes));
    const sha256 = bytesToHex(digest);

    const filename = outPath.endsWith(".json")
        ? (target === "linux-x86_64"
            ? "libcpp_bindings_linux.so"
            : "libcpp_bindings_linux.so")
        : "libcpp_bindings_linux.so";

    const payload = {
        target,
        filename,
        encoding: "base64" as const,
        sha256,
        data: base64FromBytes(bytes),
    };

    await Deno.writeTextFile(outPath, JSON.stringify(payload));
    console.log(`Wrote ${outPath} (${bytes.length} bytes, sha256=${sha256})`);
}
