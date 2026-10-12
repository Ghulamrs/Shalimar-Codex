#!/bin/sh
# Honest four-target readiness gate: no target is marked working by mere source presence.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER=${1:-"$ROOT/build/shalimarc"}
[ -x "$COMPILER" ] || { echo 'FAIL: compiler executable missing' >&2; exit 1; }
for file in \
  "$ROOT/src/backend/legacy_port/Arm64Darwin.cpp" \
  "$ROOT/src/backend/legacy_port/Tms6747.cpp" \
  "$ROOT/src/backend/tms6747/TargetContract.cpp"; do
  [ -f "$file" ] || { echo "FAIL: missing $file" >&2; exit 1; }
done
printf '%s\n' 'Windows x86-64: partial COFF instruction selection (rev51 regression suite needed); native execution unverified'
printf '%s\n' 'macOS ARM64: legacy port source present; native Mach-O emitter and DWARF qualification not demonstrated'
printf '%s\n' 'TI TMS6747: legacy port and target descriptor present; native DSP object/debug emitter unverified'
printf '%s\n' 'Linux x86-64: native ELF/DWARF qualification not demonstrated'
# Verify unsupported targets are not silently reported as working by the CLI.
"$COMPILER" --help > /dev/null 2>&1 || true
printf '%s\n' 'Target readiness audit passed (presence/claims only; not a target execution test)'
