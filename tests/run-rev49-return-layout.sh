#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
${CXX:-c++} -std=c++98 -pedantic-errors -Wall -Wextra -I"$ROOT/include" "$ROOT/tests/rev49_return_layout.cpp" -o "$TMP/layout"
"$TMP/layout"
echo 'rev49 typed multi-return layout tests passed'
