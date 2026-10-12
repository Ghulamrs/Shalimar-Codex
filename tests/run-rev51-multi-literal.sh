#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"$COMPILER" "$ROOT/tests/rev51_multi_literal.shl" --check
for mode in debug release; do
  "$COMPILER" "$ROOT/tests/rev51_multi_literal.shl" --object "$TMP/$mode.obj" "--$mode"
  objdump -d "$TMP/$mode.obj" > "$TMP/$mode.asm"
  grep -q '<three>:' "$TMP/$mode.asm"
  grep -q 'mov.*%eax,0x0(%rcx)' "$TMP/$mode.asm"
  grep -q 'mov.*%rax,0x8(%rcx)' "$TMP/$mode.asm"
  grep -q 'mov.*%eax,0x10(%rcx)' "$TMP/$mode.asm"
  grep -q 'mov.*%rcx,%rax' "$TMP/$mode.asm"
done
echo 'rev51 literal multi-return COFF Debug/Release disassembly tests passed'
