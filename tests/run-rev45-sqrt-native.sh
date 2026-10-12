#!/bin/sh
set -eu
COMPILER="${1:-./build/shalimarc}"
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
"$COMPILER" "$ROOT/tests/rev45_sqrt_literal.shl" --object "$TMP/root.obj" --debug
objdump -r "$TMP/root.obj" | grep 'IMAGE_REL_AMD64_REL32.*sqrt'
objdump -d "$TMP/root.obj" | grep 'movq.*%xmm0'
"$COMPILER" "$ROOT/tests/rev45_sqrt_literal.shl" --object "$TMP/release.obj" --release
objdump -r "$TMP/release.obj" | grep 'IMAGE_REL_AMD64_REL32.*sqrt'
# A missing uses declaration must not authorize the external symbol.
sed '/^uses sqrt$/d' "$ROOT/tests/rev45_sqrt_literal.shl" > "$TMP/no_uses.shl"
if "$COMPILER" "$TMP/no_uses.shl" --object "$TMP/invalid.obj" >/dev/null 2>&1; then
    echo 'ERROR: unimported sqrt accepted' >&2
    exit 1
fi
echo 'rev45 native sqrt COFF regression passed'
