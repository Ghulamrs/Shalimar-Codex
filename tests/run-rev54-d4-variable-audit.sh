#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
for MODE in debug release; do
  "$COMPILER" "$ROOT/tests/rev54_codeview_probe.shl" --object "$TMP/probe.obj" "--$MODE" --debug-symbols --debug-variables "$TMP/variables.tsv"
  grep -q '^SHALIMAR-DEBUG-VARIABLES' "$TMP/variables.tsv"
  # A function without declarations must not manufacture variable locations.
  test "$(wc -l < "$TMP/variables.tsv")" -eq 1
  if "$COMPILER" "$ROOT/tests/source_mapping.shl" --object "$TMP/locals.obj" "--$MODE" --debug-symbols --debug-variables "$TMP/locals.tsv" >"$TMP/rejected.log" 2>&1; then
    echo 'FAIL: expected unsupported production local-variable codegen to be rejected' >&2; exit 1
  fi
  grep -q 'supports only zero-argument int functions with one basic block' "$TMP/rejected.log"
  test ! -f "$TMP/locals.tsv"
done
echo 'D4.2 variable audit PASS: honest empty metadata, production local fixture still unsupported (Debug/Release)'
