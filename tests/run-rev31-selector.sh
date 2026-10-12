#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++98 -Wall -Wextra -pedantic -I"$ROOT/include" \
  "$ROOT/src/backend/x86_64/InstructionEncoder.cpp" \
  "$ROOT/src/backend/x86_64/IRSelector.cpp" \
  "$ROOT/src/IRVerifier.cpp" \
  "$ROOT/src/backend/tms6747/TargetContract.cpp" \
  "$ROOT/tests/rev31_selector.cpp" -o "$TMP/rev31"
"$TMP/rev31"
echo 'rev31 restricted IR selector and DSP target declaration: PASS'
