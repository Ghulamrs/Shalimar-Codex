# Rev51–rev52 closure audit (2026-10-11)

## Confirmed
- The obsolete char(65) return-expression regression fixture was replaced by a legal partial return (`return (12)`), testing typed trailing zeros without weakening the prohibition on calls in return arguments.
- The existing rev51 regression scripts and rev52 trailing-zero native object test pass with the current C++98 compiler.

## Not closed
- The Windows x64 selector still contains pattern-specific lowering and cannot handle general runtime expression trees.
- General multi-result assignment and nested/repeated calls are not proven by executable-level tests.
- COFF disassembly checks do not establish Windows runtime correctness; Linux/macOS/TMS6747 are not qualified.
- No Windows executable execution or full specification conformance report has been produced.

Do not label rev51 or rev52 complete until the above requirements are implemented and verified.

## 2026-10-11 runtime multiplication checkpoint

Added IMUL EAX,EAX,imm32 to the x86-64 encoder and supported binary multiplication in the bounded parameter-based SSA arithmetic lowering path. The test `tests/run-rev51-multi-runtime-multiply.sh` checks generated Debug and Release COFF disassembly. Strict C++98 build succeeded; 15 rev51/rev52 shell scripts passed, zero failed. This is not general expression lowering, general multi-assignment, linked executable runtime verification, or rev51/rev52 closure.
