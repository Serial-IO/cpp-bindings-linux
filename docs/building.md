# Building the shared library

## 1. Prerequisites

* **Compiler:** `g++` or `clang++` with C++23 support
* **CMake ≥ 3.30**
* POSIX development headers (`termios.h`, `fcntl.h`, …)

Example install on Debian/Ubuntu:

```bash
sudo apt-get update
sudo apt-get install build-essential cmake clang make
```

---

## 2. Configure & Compile

```bash
# From the repository root
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

This produces:

* `build/libcpp_unix_bindings.so` – the shared library
* `build/tests` – aggregated gtest binary (run via `ctest` or `./tests`)
* `compile_commands.json` – copied to project root for clang-tools

### Debug build

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug -j
```

### Installing system-wide

```bash
sudo cmake --install build
```

Set `CMAKE_INSTALL_PREFIX` to control the destination directory.

---

## 3. Troubleshooting

| Symptom | Possible Fix |
|---------|--------------|
| *Linker cannot find `stdc++fs`* | Ensure GCC ≥ 8 or Clang ≥ 10 |
| *Undefined reference to `cfsetspeed`* | Make sure `-pthread` is not stripped |
| *`EACCES` when opening `/dev/ttyUSB0`* | Add your user to the `dialout` (Ubuntu) or `uucp` (Arch) group or run via `sudo` | 
