#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER="${1:-$ROOT/build/shalimarc}"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
for MODE in debug release; do
  "$COMPILER" "$ROOT/tests/rev45_sqrt_literal.shl" --object "$TMP/test.obj" "--$MODE" --debug-map "$TMP/map.tsv"
  grep -q '^SHALIMAR-NATIVE-DEBUG-MAP' "$TMP/map.tsv"
  test -s "$TMP/test.obj"
  if "$COMPILER" "$ROOT/tests/rev45_sqrt_literal.shl" --object "$TMP/test.obj" --debug-symbols 2>/dev/null; then
    echo 'ERROR: CodeView falsely accepted' >&2; exit 1
  fi
done
echo 'rev48: native source offset manifest and CodeView rejection passed'
