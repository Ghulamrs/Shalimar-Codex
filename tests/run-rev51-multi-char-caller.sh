#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"$COMPILER" "$ROOT/tests/rev51_multi_char_caller.shl" --check
for mode in debug release; do
  "$COMPILER" "$ROOT/tests/rev51_multi_char_caller.shl" --object "$TMP/$mode.obj" "--$mode"
  objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
  objdump -r "$TMP/$mode.obj" > "$TMP/$mode.reloc"
  grep -q '<caller_char>:' "$TMP/$mode.asm"
  grep -q 'mov.*0x24(%rsp),%al' "$TMP/$mode.asm"
  grep -q 'mov.*%al,0x4(%rcx)' "$TMP/$mode.asm"
  grep -q 'three_char' "$TMP/$mode.reloc"
done
echo 'rev51 char caller Debug/Release object tests passed'
