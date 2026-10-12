#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"$COMPILER" "$ROOT/tests/rev51_multi_runtime_multiply.shl" --check
for mode in debug release; do
  "$COMPILER" "$ROOT/tests/rev51_multi_runtime_multiply.shl" --object "$TMP/$mode.obj" "--$mode"
  objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
  grep -q 'imul.*\$0x6' "$TMP/$mode.asm"
  grep -q 'imul.*\$0x3' "$TMP/$mode.asm"
  grep -q 'sub.*\$0x4' "$TMP/$mode.asm"
done
echo 'rev51 runtime multiplication and mixed arithmetic: Debug/Release object checks passed'
