#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
for MODE in debug release; do
  for BACKEND in ast ir; do
    EXTRA=
    if [ "$BACKEND" = ir ]; then EXTRA=--ir-native; fi
    "$COMPILER" "$ROOT/tests/multi_output.shl" --emit-c "$TMP/$MODE-$BACKEND.c" "--$MODE" $EXTRA
    cc -std=c99 "$TMP/$MODE-$BACKEND.c" -lm -o "$TMP/$MODE-$BACKEND"
    ACTUAL=$("$TMP/$MODE-$BACKEND")
    if [ "$ACTUAL" != '3 2 ' ] && [ "$ACTUAL" != '3 2' ]; then
      printf 'FAIL: %s %s output <%s>\n' "$MODE" "$BACKEND" "$ACTUAL" >&2
      exit 1
    fi
  done
done
printf 'PASS: multi-output ABI gate\n'
