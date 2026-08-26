#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <binary-path> <linux-x86_64-gnu|linux-aarch64-gnu>" >&2
    exit 1
fi

BINARY_PATH=$1
TARGET=$2
MAX_GLIBC_VERSION=2.28

if [ ! -f "$BINARY_PATH" ]; then
    echo "Binary does not exist: $BINARY_PATH" >&2
    exit 1
fi

case "$TARGET" in
    linux-x86_64-gnu)
        EXPECTED_MACHINE="Advanced Micro Devices X86-64"
        ;;
    linux-aarch64-gnu)
        EXPECTED_MACHINE="AArch64"
        ;;
    *)
        echo "Unsupported target: $TARGET" >&2
        exit 1
        ;;
esac

ACTUAL_MACHINE=$(readelf --file-header "$BINARY_PATH" | sed -n 's/^ *Machine: *//p')
if [ "$ACTUAL_MACHINE" != "$EXPECTED_MACHINE" ]; then
    echo "Expected ELF machine '$EXPECTED_MACHINE', got '$ACTUAL_MACHINE'" >&2
    exit 1
fi

if readelf --dynamic "$BINARY_PATH" | grep -Eq 'Shared library: \[(libstdc\+\+|libgcc_s)'; then
    echo "Release binary must not depend on the target system's C++ or GCC runtime" >&2
    exit 1
fi

GLIBC_VERSIONS=$(readelf --version-info "$BINARY_PATH" | grep -o 'GLIBC_[0-9][0-9.]*' || true)
if [ -n "$GLIBC_VERSIONS" ]; then
    HIGHEST_GLIBC_VERSION=$(printf '%s\n' "$GLIBC_VERSIONS" | sed 's/^GLIBC_//' | sort -Vu | tail -n 1)
    HIGHEST_ALLOWED_VERSION=$(printf '%s\n%s\n' "$MAX_GLIBC_VERSION" "$HIGHEST_GLIBC_VERSION" | sort -Vu | tail -n 1)

    if [ "$HIGHEST_ALLOWED_VERSION" != "$MAX_GLIBC_VERSION" ]; then
        echo "Release binary requires glibc $HIGHEST_GLIBC_VERSION; maximum is $MAX_GLIBC_VERSION" >&2
        exit 1
    fi
else
    HIGHEST_GLIBC_VERSION=none
fi

if [ "$TARGET" = "linux-x86_64-gnu" ]; then
    X86_PROPERTIES=$(readelf --notes "$BINARY_PATH" 2>/dev/null)
    if printf '%s\n' "$X86_PROPERTIES" | grep -Eq 'x86-64-v[234]'; then
        echo "x86-64 release binary requires a higher-than-baseline ISA" >&2
        exit 1
    fi
fi

echo "Verified $TARGET: machine=$ACTUAL_MACHINE, max-glibc=$HIGHEST_GLIBC_VERSION"
