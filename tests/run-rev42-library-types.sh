#!/usr/bin/env bash
set -euo pipefail
compiler="$1"
root="$(cd "$(dirname "$0")/.." && pwd)"
"$compiler" "$root/tests/library_functions.shl" --check --debug
"$compiler" "$root/tests/library_functions.shl" --check --release
if "$compiler" "$root/tests/library_functions.shl" --object /tmp/shalimar-rev42.obj 2>/tmp/shalimar-rev42.err; then
  echo 'FAIL: unimplemented native library calls accepted' >&2; exit 1
fi
grep -q 'library imports are recorded' /tmp/shalimar-rev42.err
printf 'rev42 typed library call regression passed\n'
