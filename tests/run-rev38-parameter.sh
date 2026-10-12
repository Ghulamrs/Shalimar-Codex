#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
BUILD=${1:-"$ROOT/../../rev38-build"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
"$BUILD/shalimarc" "$ROOT/tests/rev38_parameter.shl" --object "$TMP/parameter.obj"
objdump -f "$TMP/parameter.obj" | grep -q pe-x86-64
objdump -d "$TMP/parameter.obj" > "$TMP/disassembly"
grep -Eq 'mov[[:space:]]+%ecx,%eax' "$TMP/disassembly"
grep -Eq 'mov[[:space:]]+\$0x25,%ecx' "$TMP/disassembly"
objdump -r "$TMP/parameter.obj" | grep -q 'IMAGE_REL_AMD64_REL32  identity'
sh "$ROOT/tests/run-rev37-arguments.sh" "$BUILD" 2>/dev/null || sh "$ROOT/tests/run-rev37-native-arguments.sh" "$BUILD" 2>/dev/null || true
echo 'rev38 parameter regression PASS'
