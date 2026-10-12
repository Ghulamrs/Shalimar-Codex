#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++98 -Wall -Wextra -pedantic -I"$ROOT/include" \
  "$ROOT/src/backend/x86_64/InstructionEncoder.cpp" "$ROOT/tests/rev30_encoder.cpp" -o "$TMP/encoder_test"
"$TMP/encoder_test"
echo 'rev30 x86-64 encoder and source offset tests: PASS'
