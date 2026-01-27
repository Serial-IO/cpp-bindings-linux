#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <binaryFilePath> <destinationFilePath>" >&2
  exit 1
fi

BINARY_FILE_PATH=$1
DESTINATION_FILE_PATH=$2

if [ ! -f "$BINARY_FILE_PATH" ]; then
  echo "Error: Binary path is not a file: $BINARY_FILE_PATH" >&2
  exit 1
fi

mkdir -p "$(dirname "$DESTINATION_FILE_PATH")"

cp "$BINARY_FILE_PATH" "$DESTINATION_FILE_PATH"
