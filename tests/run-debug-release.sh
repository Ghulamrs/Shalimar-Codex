#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR=${SHALIMAR_MATRIX_DIR:-"$ROOT/build-matrix"}
mkdir -p "$BUILD_DIR"
for build in debug release; do
    mkdir -p "$BUILD_DIR/$build"
    if [ "$build" = debug ]; then flags='-g -O0'; else flags='-O2 -DNDEBUG'; fi
    g++ -std=c++98 -Wall -Wextra -pedantic $flags -I"$ROOT/include" "$ROOT"/src/*.cpp -o "$BUILD_DIR/$build/shalimarc"
    for mode in debug release; do
        python3 "$ROOT/tests/run-original.py" --compiler "$BUILD_DIR/$build/shalimarc" --mode "$mode" --output "$BUILD_DIR/$build/$mode"
        python3 "$ROOT/tests/run-conformance.py" --compiler "$BUILD_DIR/$build/shalimarc" --mode "$mode" --output "$BUILD_DIR/$build/conformance-$mode"
    done
    SHALIMAR_COMPILER="$BUILD_DIR/$build/shalimarc" SHALIMAR_BUILD_DIR="$BUILD_DIR/$build/smoke" sh "$ROOT/tests/run-smoke.sh"
done
python3 "$ROOT/tests/audit-allocation.py" > "$BUILD_DIR/allocation-audit.json"
g++ -std=c++98 -Wall -Wextra -pedantic -I"$ROOT/include" "$ROOT/tests/arena_lifetime.cpp" -o "$BUILD_DIR/arena_lifetime"
"$BUILD_DIR/arena_lifetime"
