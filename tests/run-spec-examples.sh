#!/bin/sh
# Original Shalimar specification examples: syntax, semantics, and C compilation.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${SHALIMAR_COMPILER:-"$ROOT/../build/shalimarc"}
CC=${CC:-cc}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
count=0
for source in "$ROOT"/tests/spec-examples/*.shm; do
    name=$(basename "$source" .shm)
    "$COMPILER" "$source" --check >/dev/null
    for mode in debug release; do
        "$COMPILER" "$source" --emit-c "$TMP/$name-$mode.c" "--$mode" >/dev/null
        "$CC" -std=c99 -c "$TMP/$name-$mode.c" -o "$TMP/$name-$mode.o"
    done
    count=$((count + 1))
done
printf 'Specification examples: %s/%s check and Debug/Release C compile PASS\n' "$count" "$count"
