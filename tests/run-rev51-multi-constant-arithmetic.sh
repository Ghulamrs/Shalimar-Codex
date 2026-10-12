#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"$COMPILER" "$ROOT/tests/rev51_multi_constant_arithmetic.shl" --check
for mode in debug release; do
    "$COMPILER" "$ROOT/tests/rev51_multi_constant_arithmetic.shl" --object "$TMP/$mode.obj" "--$mode"
    objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
    grep -q '\$0x11,%eax' "$TMP/$mode.asm"
    grep -q '\$0x14,%eax' "$TMP/$mode.asm"
done
echo 'rev51 constant SSA arithmetic: Debug/Release COFF passed'
