#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
c++ -std=c++98 -pedantic-errors -Wall -Wextra -I"$ROOT/include" "$ROOT/tests/rev35_coff_relocations.cpp" "$ROOT/src/backend/coff/CoffWriter.cpp" -o "$TMP/test"
"$TMP/test" "$TMP/test.obj"
objdump -f "$TMP/test.obj" | grep -q 'pe-x86-64'
objdump -r "$TMP/test.obj" | grep -q 'IMAGE_REL_AMD64_REL32'
objdump -r "$TMP/test.obj" | grep -q 'callee'
objdump -r "$TMP/test.obj" | grep -q 'external_long_name'
objdump -t "$TMP/test.obj" | grep -q 'caller'
echo 'rev35 COFF relocation regression PASS'
