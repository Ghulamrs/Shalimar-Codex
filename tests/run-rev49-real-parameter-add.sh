#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
for mode in debug release; do
  "$COMPILER" "$ROOT/tests/rev49_real_parameter_add.shl" --object "$TMP/$mode.obj" "--$mode"
  objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
  grep -q '<addOffset>:' "$TMP/$mode.asm"
  grep -q '<addReversed>:' "$TMP/$mode.asm"
  test "$(grep -c 'addsd.*%xmm1,%xmm0' "$TMP/$mode.asm")" -eq 2
  test "$(grep -c 'movq.*%xmm1' "$TMP/$mode.asm")" -eq 2
done
printf 'rev49 real parameter plus literal native tests passed\n'
