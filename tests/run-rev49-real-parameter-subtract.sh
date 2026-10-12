#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
for mode in debug release; do
  "$COMPILER" "$ROOT/tests/rev49_real_parameter_subtract.shl" --object "$TMP/$mode.obj" "--$mode"
  objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
  grep -q '<subtractOffset>:' "$TMP/$mode.asm"
  grep -q 'subsd.*%xmm1,%xmm0' "$TMP/$mode.asm"
done
printf 'rev49 real parameter subtraction native tests passed\n'
