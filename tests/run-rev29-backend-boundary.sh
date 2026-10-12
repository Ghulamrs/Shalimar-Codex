#!/bin/sh
set -eu
compiler="$1"
root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
"$compiler" "$root/tests/multi_output.shl" --check --middle=s >/dev/null
"$compiler" "$root/tests/multi_output.shl" --dump-ir --middle=s >/dev/null
if "$compiler" "$root/tests/multi_output.shl" --emit-obj --middle=s --debug-symbols 2>"${TMPDIR:-/tmp}/shalimar-rev29-error.log"; then
  echo 'Unexpected native output success' >&2; exit 1
fi
grep -q 'no C fallback' "${TMPDIR:-/tmp}/shalimar-rev29-error.log"
echo 'rev29 backend boundary tests passed'
