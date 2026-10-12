# Rev3 repeat review and first repair tranche — 11 October 2026

The review covers Windows x64, Linux x64, macOS ARM64 and TMS6747. The four targets do not yet implement equivalent language subsets. Repairs were applied to the extracted Rev3 compiler source; the earlier source snapshot and ZIP archives were not modified.

## Fixed and verified

- Shared semantic checking preserves borrowed-parameter errors, rejects duplicate parameter names and reports statement line numbers.
- The driver explicitly accepts all four target names. Linux/macOS native output is rejected as unimplemented instead of presenting unknown-option behavior or implying Windows output qualifies those targets. Frontend success explicitly says it does not validate target code generation or native debugging.
- DSP rejects Windows-only debug report flags and incompatible source-index modes. DSP Release emits requested IR optimization counts. Symbols requested during frontend-only checks are rejected.
- IR lowering stops after terminators. Return blocks have no trailing jumps or successor edges. The verifier rejects instructions after a terminator and mismatched successor counts.
- DSP uses a must-initialization data-flow analysis. A read must be initialized on every reachable predecessor path. Tests include bare uninitialized locals, one-sided branch initialization, initialization on both branches and possibly zero-trip loops. Rejection is an interim native restriction pending explicit scalar-default semantics.
- Shared known-integer overflow validation runs before machine selection. Windows selector constant evaluation uses checked signed arithmetic instead of unsigned wraparound. Literal and same-block initialized-local overflow cases are rejected in source Debug and Release on both active targets. Runtime-input overflow is not fixed by these checks.
- Integer propagation no longer substitutes declared real/char/array symbols with integer facts. The repeat review identified real locals initialized by integer literals as a source of wrong Release arithmetic types. The repaired DSP produces the exact binary64 result 2147483648.0 for `real x : 2147483647; return x + 1` (using Shalimar statement syntax) in all four host/source build combinations.
- The README now describes the native driver and clearly labels old C-backend instructions as historical. C++ test assertions are retained with the compiler-appropriate flag.

## Fresh validation

| Validation | Result |
|---|---:|
| Fresh MSVC host Debug and Release builds | Both pass |
| C++ contracts, Debug | 14/14 |
| C++ contracts, Release | 14/14 |
| Cross-target CLI cases, Debug host | 30/30 |
| Cross-target CLI cases, Release host | 30/30 |
| Windows native DLL executions, four host/source combinations | 108/108 |
| Explicit Windows export result assertions | 140/140 |
| DSP emit/assemble/link + VM and SIM6747 checks, four combinations | 52/52 |
| Selected corpus stage invocations | 924 completed |

The selected corpus retains exactly the previous acceptance sets: 132/154 frontend/IR, 27/154 Windows object, 21/154 DSP assembly, in both source modes. No stage acceptance regression was observed. All original 12 examples still pass the frontend and neither native target emits them. The new regressions are additional to this historical corpus.

DSP has 12 existing fixtures plus the real-integer-initializer regression. VM checks exact integer/binary64 result words. SIM6747 checks full integer status and termination; its wrapper returns zero for real results, so real numeric correctness is established by VM rather than inferred from simulator exit status. Windows uses the supplied sqrt test shim, not a production CRT implementation. CCS source-debugger qualification was not rerun.

Evidence is retained in the workspace `compiler-review/cli-repairs-debug`, `cli-repairs-release`, `repair-native`, `repair-corpus`, and build logs. `tests/rev3_repair_contract.cpp` and `tests/run-rev3-repairs.py` are the committed-source regression entry points; `compiler-review/verify-repairs-native.py` reproduces the native execution checks with the bundled own DSP tools and installed Microsoft linker.

## Remaining defects and gaps

**Runtime-input integer overflow remains a correctness blocker.** Fresh post-repair Windows calls with INT_MAX and a DSP function receiving INT_MAX still return INT_MIN without a language diagnostic. Evidence is `compiler-review/repair-native/remaining-runtime-overflow.json`. A target runtime error ABI, checked machine operations and diagnostic preservation are the next priority; these results must not be represented as complete arithmetic safety.

Linux/macOS have explicit frontend-only target selection but no integrated native output. Windows/DSP retain unequal language coverage. Arrays, references, globals, text/printing, general library calls and several operations need end-to-end implementation. Windows unwind/debugger completeness, own DSP DWARF, multiple semantic diagnostics and warnings, type-complete IR verification, and canonical-specification reconciliation also remain open.

The first tranche is complete and tested. Full compiler conformance is not complete. Follow `REV3_REPAIR_PLAN.md` for the remaining target and correctness gates.
