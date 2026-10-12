#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
for mode in debug release; do
    "$COMPILER" "$ROOT/tests/rev49_real_literal.shl" --object "$TMP/$mode.obj" "--$mode"
    objdump -d "$TMP/$mode.obj" | grep -q 'movq.*%xmm0'
    objdump -d "$TMP/$mode.obj" | grep -q 'ret'
done
printf 'rev49 real literal native return passed\n'
