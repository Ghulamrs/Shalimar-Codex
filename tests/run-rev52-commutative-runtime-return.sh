#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"$COMPILER" "$ROOT/tests/rev52_commutative_runtime_return.shl" --check
for mode in debug release; do
  "$COMPILER" "$ROOT/tests/rev52_commutative_runtime_return.shl" --object "$TMP/$mode.obj" "--$mode"
  objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
  grep -q 'imul.*\$0x3' "$TMP/$mode.asm"
  grep -q 'imul.*\$0x4' "$TMP/$mode.asm"
  grep -q 'add.*\$0x7' "$TMP/$mode.asm"
done
echo 'rev52 commutative runtime return: Debug/Release object checks passed'
