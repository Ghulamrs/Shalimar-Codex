#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
for mode in debug release; do
    "$COMPILER" "$ROOT/tests/rev49_real_identity_add.shl" --object "$TMP/$mode.obj" "--$mode"
    objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
    grep -q '<identity>:' "$TMP/$mode.asm"
    grep -q '<plus>:' "$TMP/$mode.asm"
    grep -q 'movq.*%xmm0' "$TMP/$mode.asm"
    grep -q '400e000000000000' "$TMP/$mode.asm"
done
printf 'rev49 real identity and constant-add native tests passed\n'
