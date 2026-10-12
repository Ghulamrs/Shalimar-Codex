#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"$COMPILER" "$ROOT/tests/rev51_multi_real_caller.shl" --check
for mode in debug release; do
  "$COMPILER" "$ROOT/tests/rev51_multi_real_caller.shl" --object "$TMP/$mode.obj" "--$mode"
  objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
  objdump -r "$TMP/$mode.obj" > "$TMP/$mode.reloc"
  grep -q '<caller_real>:' "$TMP/$mode.asm"
  grep -q 'movsd.*0x28(%rsp),%xmm0' "$TMP/$mode.asm"
  grep -q 'three' "$TMP/$mode.reloc"
done
echo 'rev51 real caller Debug/Release object tests passed'
