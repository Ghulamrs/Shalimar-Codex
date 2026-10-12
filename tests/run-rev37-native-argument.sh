#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
BUILD=${1:-"$ROOT/../../rev37-build"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
"$BUILD/shalimarc" "$ROOT/tests/rev37_arg.shl" --object "$TMP/argument.obj"
objdump -f "$TMP/argument.obj" | grep -q pe-x86-64
objdump -d "$TMP/argument.obj" | grep -Eq 'mov[[:space:]]+\$0x25,%ecx'
objdump -r "$TMP/argument.obj" | grep -q 'IMAGE_REL_AMD64_REL32  fortyTwo'
sh "$ROOT/tests/run-rev36-native-calls.sh" "$BUILD"
echo 'rev37 native argument regression PASS'
