#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER="${1:-$ROOT/build/shalimarc}"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
for MODE in debug release; do
  "$COMPILER" "$ROOT/tests/rev54_codeview_probe.shl" --object "$TMP/test.obj" "--$MODE" --debug-symbols
  objdump -h "$TMP/test.obj" | grep -q '\.debug\$S'
  objdump -r "$TMP/test.obj" | grep -q IMAGE_REL_AMD64_SECREL
  lld-link /dll /noentry /nodefaultlib /debug /out:"$TMP/test.dll" /pdb:"$TMP/test.pdb" "$TMP/test.obj"
  test -s "$TMP/test.pdb"
done
echo 'rev54: C13 COFF section, line relocations, and PDB link passed (not debugger validated)'
