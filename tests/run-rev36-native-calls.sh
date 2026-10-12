#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
BUILD=${1:-"$ROOT/../../rev36-build"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
"$BUILD/shalimarc" "$ROOT/tests/rev36_call.shl" --object "$TMP/calls.obj"
objdump -f "$TMP/calls.obj" | grep -q pe-x86-64
objdump -r "$TMP/calls.obj" | grep -q 'IMAGE_REL_AMD64_REL32  answer'
objdump -d "$TMP/calls.obj" | grep -q 'call'
objdump -t "$TMP/calls.obj" | grep -q 'caller'
echo 'rev36 native call regression PASS'
