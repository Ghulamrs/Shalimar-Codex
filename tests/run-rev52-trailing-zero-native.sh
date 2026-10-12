#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"$COMPILER" "$ROOT/tests/return_trailing_zero_valid.shl" --check
for mode in debug release; do
  "$COMPILER" "$ROOT/tests/return_trailing_zero_valid.shl" --object "$TMP/$mode.obj" "--$mode"
  objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
  grep -q '<partial>:' "$TMP/$mode.asm"
  grep -q '<empty>:' "$TMP/$mode.asm"
  grep -q 'mov.*%al,0x10(%rcx)' "$TMP/$mode.asm"
  grep -q 'mov.*%eax,0x14(%rcx)' "$TMP/$mode.asm"
done
echo 'rev52 trailing zero native stores: Debug/Release passed'
