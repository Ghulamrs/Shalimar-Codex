#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"$COMPILER" "$ROOT/tests/rev51_multi_caller.shl" --check
for mode in debug release; do
  "$COMPILER" "$ROOT/tests/rev51_multi_caller.shl" --object "$TMP/$mode.obj" "--$mode"
  objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
  objdump -r "$TMP/$mode.obj" > "$TMP/$mode.reloc"
  grep -q '<caller>:' "$TMP/$mode.asm"
  grep -q 'lea.*0x20(%rsp),%rcx' "$TMP/$mode.asm"
  grep -q 'mov.*0x20(%rsp),%eax' "$TMP/$mode.asm"
  grep -q 'three' "$TMP/$mode.reloc"
done
echo 'rev51 caller-side multi-return Debug/Release object tests passed'
