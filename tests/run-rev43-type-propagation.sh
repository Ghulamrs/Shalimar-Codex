#!/usr/bin/env bash
set -euo pipefail
compiler="$1"
root="$(cd "$(dirname "$0")/.." && pwd)"
for mode in --debug --release; do
    output="$($compiler "$root/tests/rev43_type_propagation.shl" --dump-ir "$mode")"
    count="$(printf '%s\n' "$output" | grep -c 'sqrt(real) -> real')"
    test "$count" -eq 2 || { echo 'expected two resolved sqrt(real) calls' >&2; exit 1; }
done
echo 'rev43 type propagation passed'
