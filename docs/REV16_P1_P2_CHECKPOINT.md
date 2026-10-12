# Revision 16 — P1/P2 backend-parity checkpoint

## Implemented

- `splitIRArguments` in `src/IRCodeGenerator.cpp` now respects quoted commas and escaped quote characters. The former implementation split on every comma.
- Preserved standard C++98 compilation and existing compiler behavior.

## Verified

- Linux CMake C++98 build: PASS.
- Existing `tests/run-smoke.sh`: PASS.
- The parity probes and their exact diagnostics are recorded in `docs/REV16_PARITY_PROBES.json`.

## Not completed

- Passing a string literal directly to a rank-one `char` parameter in IR-native code generation.
- General cross-file `uses` (currently borrowed library functions only).
- IR-native multi-output functions.
- Full higher-rank array support.
- Optional `char[]` library functions (`strlen`, `find`, `substr`, `upper`, `lower`).
- Exhaustive integer overflow, conversion-boundary, and non-finite arithmetic audits. Existing smoke tests cover important cases but do not establish exhaustiveness.
- P0 elimination of `AstRef`/`astCast` remains outstanding.

This archive is a tested development checkpoint, NOT a finished P1/P2 release.
