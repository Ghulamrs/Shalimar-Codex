# Rev51–rev52 first closure status — 2026-10-11

## Verified in this checkpoint

- CMake compiler build succeeded with the existing strict C++98 configuration.
- All ten rev51 test scripts passed, including the new nested arithmetic test. The target-readiness script audits presence/claims only.
- Windows x64 COFF object disassembly shows both independent nested chains `x + 10 + 5` and `x - 7 - 2` lowered and stored into typed result fields.
- A bounded (64-level) recursive IR temporary resolver now handles chained additions/subtractions rooted at the first integer argument, with an immediate integer right operand. It rejects other shapes rather than emitting guessed instructions.

## Not verified / not closed

- rev51: general expression trees, multiplication/division, multi-argument calls, repeated/nested calls, unrestricted multiple assignment, executable-level correctness, stack unwind and full ABI conformance.
- rev52: comprehensive language-specification parity across the native backend, arrays/strings/global data, control flow, functions and all semantic error cases.
- No target runtime execution was performed. COFF disassembly is not executable-level evidence.
- macOS ARM64 and TI TMS6747 remain mandatory future targets, not implemented/qualified by this checkpoint.

## Release gate

Do not label rev51 or rev52 complete until a feature matrix based on the normative language reference has passing native object, link and runtime tests for every supported construct, including negative diagnostics and Debug/Release equivalence.

## Follow-up checkpoint: constant SSA chains

- The Windows x64 native selector now evaluates constant-only `binary +` and `binary -` SSA chains for individual integer fields of a multiple-result return structure.
- A new regression test checks `(10 + 5 + 2, 30 - 7 - 3)` with Debug/Release COFF disassembly.
- This is deliberately limited to compile-time-resolvable expressions; arbitrary mixed parameter/constant trees, general multi-assignment, runtime execution, and full rev52 parity remain open.
