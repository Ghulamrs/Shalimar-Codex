#!/bin/sh
set -eu
COMPILER="${1:-./shalimarc}"
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
"$COMPILER" "$HERE/return_42.shl" -S "$TMP/dsp.asm" --target=tms6747 --debug --dsp-debug-index "$TMP/debug.idx"
grep -q 'MVKL[[:space:]]*42, A4' "$TMP/dsp.asm"
grep -q 'function[[:space:]]*answer' "$TMP/debug.idx"
"$COMPILER" "$HERE/return_42.shl" --object "$TMP/windows.obj" --debug
test -s "$TMP/windows.obj"
if "$COMPILER" "$HERE/return_42.shl" --object "$TMP/invalid.obj" --target=tms6747; then
  echo 'ERROR: DSP object mode must be rejected until TI tools are integrated' >&2
  exit 1
fi
echo 'PASS: DSP assembly, debug index, Windows object preservation, DSP object rejection'
