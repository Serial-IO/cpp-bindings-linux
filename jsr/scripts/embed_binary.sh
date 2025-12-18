#!/bin/sh
set -eu

BINARY_PATH="$1"
TARGET="$2"

FILENAME=$(basename ${BINARY_PATH})

if [ -z "${BINARY_PATH:-}" ] || [ -z "${TARGET:-}" ]; then
  echo "Usage: $0 <binaryPath> <target>" >&2
  exit 1
fi

JSR_BIN_PATH="./jsr/package/bin"

mkdir -p "$JSR_BIN_PATH"

cp "$BINARY_PATH" "$JSR_BIN_PATH/x86_64.so"

# base64 without linebreak (GNU + BSD kompatibel)
BASE64_DATA="$(base64 "$BINARY_PATH" | tr -d '\n')"

cat > "$JSR_BIN_PATH/x86_64.json" <<EOF
{
  "target": "$TARGET",
  "filename": "$FILENAME",
  "encoding": "base64",
  "data": "$BASE64_DATA"
}
EOF
