#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
python3 "$ROOT/tests/conformance/run_matrix.py" "$COMPILER" --output "$ROOT/tests/conformance/rev49_results.json"
