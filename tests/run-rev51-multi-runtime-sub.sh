#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"$COMPILER" "$ROOT/tests/rev51_multi_runtime_sub.shl" --check
for mode in debug release; do
  "$COMPILER" "$ROOT/tests/rev51_multi_runtime_sub.shl" --object "$TMP/$mode.obj" "--$mode"
  objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
  grep -q '<reduced>:' "$TMP/$mode.asm"
  grep -q 'mov.*%edx,%eax' "$TMP/$mode.asm"
  grep -q 'sub.*\$0xa,%eax' "$TMP/$mode.asm"
  grep -q 'mov.*%eax,0x10(%rcx)' "$TMP/$mode.asm"
done
echo 'rev51 runtime integer subtraction in multi-return: Debug/Release object checks passed'
