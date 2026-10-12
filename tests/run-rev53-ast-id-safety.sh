#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT="${TMPDIR:-/tmp}/shalimar-rev53-ast-id-safety-$$"
trap 'rm -f "$OUT"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++98 -pedantic-errors -I "$ROOT/include" "$ROOT/tests/rev53_ast_id_safety.cpp" -o "$OUT"
"$OUT"
