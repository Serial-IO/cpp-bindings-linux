#!/bin/sh
set -eu

if [ "$#" -ne 3 ]; then
  echo "Usage: $0 <binaryFilePath> <JsonFilePath> <target>" >&2
  exit 1
fi

BINARY_FILE_PATH=$1
JSON_FILE_PATH=$2
TARGET=$3

if [ ! -f "$BINARY_FILE_PATH" ]; then
  echo "Error: Binary path is not a file: $BINARY_FILE_PATH" >&2
  exit 1
fi

BINARY_FILE_NAME=$(basename "$BINARY_FILE_PATH")

mkdir -p $(dirname "$JSON_FILE_PATH")

BASE64_DATA=$(base64 "$BINARY_FILE_PATH" | tr -d '\n')

jq -n \
  --arg target "$TARGET" \
  --arg filename "$BINARY_FILE_NAME" \
  --arg data "$BASE64_DATA" \
  '{
    target: $target,
    filename: $filename,
    encoding: "base64",
    data: $data
  }' > "$JSON_FILE_PATH"
