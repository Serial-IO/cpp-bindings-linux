#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <binaryPath> <target>" >&2
  exit 1
fi

BINARY_PATH=$1
TARGET=$2

if [ ! -f "$BINARY_PATH" ]; then
  echo "Error: binaryPath is not a file: $BINARY_PATH" >&2
  exit 1
fi

FILENAME=$(basename "$BINARY_PATH")
JSR_BIN_PATH="./jsr/package/bin"

mkdir -p "$JSR_BIN_PATH"

cp "$BINARY_PATH" "$JSR_BIN_PATH/x86_64.so"

BASE64_DATA=$(base64 "$BINARY_PATH" | tr -d '\n')

jq -n \
  --arg target "$TARGET" \
  --arg filename "$FILENAME" \
  --arg data "$BASE64_DATA" \
  '{
    target: $target,
    filename: $filename,
    encoding: "base64",
    data: $data
  }' > "$JSR_BIN_PATH/x86_64.json"
