const dataArray = Deno.readFileSync('./libcpp_bindings_linux.so')

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

Deno.writeTextFileSync('./libcpp_bindings_linux.json', base64FromBytes(dataArray))

