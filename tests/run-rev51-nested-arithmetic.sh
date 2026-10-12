#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"$COMPILER" "$ROOT/tests/rev51_nested_arithmetic.shl" --check
for mode in debug release; do
    "$COMPILER" "$ROOT/tests/rev51_nested_arithmetic.shl" --object "$TMP/$mode.obj" "--$mode"
    objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
    grep -q 'add.*\$0xa,%eax' "$TMP/$mode.asm"
    grep -q 'add.*\$0x5,%eax' "$TMP/$mode.asm"
    grep -q 'sub.*\$0x7,%eax' "$TMP/$mode.asm"
    grep -q 'sub.*\$0x2,%eax' "$TMP/$mode.asm"
    grep -q 'mov.*%eax,0x4(%rcx)' "$TMP/$mode.asm"
done
echo 'rev51 nested multi-result integer arithmetic: Debug/Release COFF disassembly passed'
