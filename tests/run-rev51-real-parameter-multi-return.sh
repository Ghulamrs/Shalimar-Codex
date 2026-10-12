#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"$COMPILER" "$ROOT/tests/rev51_real_parameter_multi_return.shl" --check
for mode in debug release; do
  "$COMPILER" "$ROOT/tests/rev51_real_parameter_multi_return.shl" --object "$TMP/$mode.obj" "--$mode"
  objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
  grep -q '<replicate_real>:' "$TMP/$mode.asm"
  grep -q 'movsd.*%xmm1,0x0(%rcx)' "$TMP/$mode.asm"
  grep -q 'movsd.*%xmm1,0x10(%rcx)' "$TMP/$mode.asm"
  grep -q 'mov.*%rcx,%rax' "$TMP/$mode.asm"
done
echo 'rev51 real-parameter multi-return Debug/Release disassembly tests passed'
