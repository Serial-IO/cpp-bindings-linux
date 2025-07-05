# Deno example scripts

| Script | Purpose |
|--------|---------|
| `serial_echo.ts` | Minimal echo test: enumerate ports, open the first one and verify that each line sent is echoed back by the MCU. |
| `serial_advanced.ts` | Demonstrates helpers (`serialReadLine`, statistics, peek, `serialDrain`) and prints TX/RX counters. |
| `serial_callbacks.ts` | Shows how to register error / read / write callbacks and prints diagnostics for every transfer. |

---

## Running an example

```bash
deno run --allow-ffi --allow-read examples/<script>.ts \
         --lib ./build/libcpp_unix_bindings.so \
         --port /dev/ttyUSB0
```

Flags:

* `--lib` (optional) – path to `libcpp_unix_bindings.so`; defaults to `./build/libcpp_unix_bindings.so`
* `--port` (optional) – serial device path; defaults to `/dev/ttyUSB0`

If your board resets on opening the port (common on Arduino), scripts wait
≈2 s before the first write. 
