#!/bin/sh
set -eu
COMPILER="${1:-./build/shalimarc}"
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
if [ "${2:-}" ]; then
  TMP="$2"
  mkdir -p "$TMP"
else
  TMP=$(mktemp -d)
  trap 'rm -rf "$TMP"' EXIT
fi
command -v clang >/dev/null
command -v lld-link >/dev/null
for MODE in debug release; do
  "$COMPILER" "$ROOT/tests/rev45_sqrt_literal.shl" --object "$TMP/shalimar.obj" "--$MODE"
  cat > "$TMP/runtime.s" <<'ASM'
.text
.globl sqrt
sqrt:
    sqrtsd %xmm0, %xmm0
    retq
.globl verify_sqrt
verify_sqrt:
    subq $40, %rsp
    callq root
    addq $40, %rsp
    movabsq $0x4022000000000000, %rax
    movq %rax, %xmm1
    ucomisd %xmm1, %xmm0
    sete %al
    setnp %cl
    andb %cl, %al
    movzbl %al, %eax
    retq
ASM
  clang -target x86_64-pc-windows-msvc -c "$TMP/runtime.s" -o "$TMP/runtime.obj"
  lld-link /dll /noentry /nodefaultlib /machine:x64 /export:verify_sqrt /out:"$TMP/shalimar-$MODE.dll" "$TMP/shalimar.obj" "$TMP/runtime.obj"
  file "$TMP/shalimar-$MODE.dll" | grep -q 'PE32+'
  objdump -p "$TMP/shalimar-$MODE.dll" | grep -q 'verify_sqrt'
  if command -v wine >/dev/null 2>&1; then
    echo 'Wine detected: execute Windows test with a configured Windows loader harness (not automatic).'
  fi
  echo "$MODE: Windows DLL linked with native numeric verification export (execution requires Windows host)"
done
