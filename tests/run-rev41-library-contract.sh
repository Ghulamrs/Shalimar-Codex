#!/usr/bin/env bash
set -euo pipefail
compiler="$1"
root="$(cd "$(dirname "$0")/.." && pwd)"
"$compiler" "$root/tests/library_functions.shl" --check --debug
"$compiler" "$root/tests/library_functions.shl" --check --release
if "$compiler" "$root/tests/library_functions.shl" --object /tmp/shalimar-rev41-library.obj 2>/tmp/shalimar-rev41-library.err; then
  echo 'FAIL: unsupported native library object unexpectedly accepted' >&2
  exit 1
fi
grep -q 'library imports are recorded' /tmp/shalimar-rev41-library.err
printf 'rev41 library contract gate passed\n'
