# rev23 — multi-output ABI implementation analysis

Status: **not implemented**. This revision adds an explicit, reproducible implementation map, not a claim of working IR-native multi-output functions.

## Current representation

`IRBuilder.cpp` emits `call <name>` into one temporary, then `multi.assign` with a comma-separated destination list and that single call temporary. `return` likewise carries comma-separated expressions in one operand. This representation is insufficient for reliable typed C emission unless the generator resolves the target function's ordered return types and the call result's tuple type.

## Required implementation

1. Create deterministic C99 `typedef struct` return types for functions with multiple results. Fields must use the declared output types, with explicit owned-copy semantics for any `char[]` field.
2. Generate prototypes and definitions returning the corresponding struct; preserve the current scalar and void ABI.
3. Track call-result tuple types by IR temporary and callee; evaluate each call exactly once.
4. Lower `multi.assign` to a temporary struct followed by field-by-field assignments, preserving order and type conversions. Handle references and aliases deliberately.
5. Lower multi-value `return` to a struct construction, preserving expression evaluation order, cleanup, and reference copy-back.
6. Add negative tests for arity/type mismatches and runtime parity tests for int/int, mixed scalar outputs, nested calls, and repeated calls.

## Acceptance gate

The original `tests/multi_output.shl` must produce `3 2` in both AST-to-C and IR-native modes; both paths must compile and run in Debug and Release. All existing smoke tests must remain green. No test source should be rewritten to mask a compiler defect.

This checkpoint does not satisfy that acceptance gate. Do not mark P1/P2 complete.
