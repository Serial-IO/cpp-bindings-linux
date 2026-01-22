#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <JsonFilePath> <version>" >&2
  exit 1
fi

JSON_FILE_PATH=$1
VERSION=$2

temp_json_file="$(mktemp)"

sed "s/\"version\": \"[^\"]*\"/\"version\": \"$VERSION\"/" \
  "$JSON_FILE_PATH" > "$temp_json_file"

mv "$temp_json_file" "$JSON_FILE_PATH"
