#!/bin/sh
set -eu
COMPILER="${1:-./build/shalimarc}"
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
command -v clang >/dev/null
command -v lld-link >/dev/null
"$COMPILER" "$ROOT/tests/rev45_sqrt_literal.shl" --object "$TMP/shalimar.obj" --debug
# Link-only test shim: independently generated native Windows x64 implementation
# of sqrt(double). It is NOT part of the Shalimar compiler or a C backend.
cat > "$TMP/sqrt.s" <<'ASM'
.text
.globl sqrt
sqrt:
    sqrtsd %xmm0, %xmm0
    retq
ASM
clang -target x86_64-pc-windows-msvc -c "$TMP/sqrt.s" -o "$TMP/sqrt.obj"
lld-link /dll /noentry /nodefaultlib /machine:x64 /export:root /out:"$TMP/shalimar.dll" "$TMP/shalimar.obj" "$TMP/sqrt.obj"
file "$TMP/shalimar.dll" | grep -E 'PE32\+.*DLL'
objdump -p "$TMP/shalimar.dll" | grep -F 'root'
# A missing symbol must fail the linker (no hidden fallback).
if lld-link /dll /noentry /nodefaultlib /machine:x64 /export:root /out:"$TMP/broken.dll" "$TMP/shalimar.obj" >"$TMP/failure.log" 2>&1; then
    echo 'ERROR: unresolved sqrt unexpectedly linked' >&2; exit 1
fi
grep -qi 'undefined symbol.*sqrt' "$TMP/failure.log"
"$COMPILER" "$ROOT/tests/rev45_sqrt_literal.shl" --object "$TMP/release.obj" --release
lld-link /dll /noentry /nodefaultlib /machine:x64 /export:root /out:"$TMP/release.dll" "$TMP/release.obj" "$TMP/sqrt.obj"
echo 'rev46 Windows x64 link regression passed (execution not tested)'
