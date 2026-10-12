# Rev3 four-target review and repair plan

Baseline: 11 October 2026, Rev3 merged source. The repeat review confirms that Windows x64 and TMS6747 have different native subsets; Linux x64 and macOS ARM64 are legacy emitters without integrated object pipelines. The detailed baseline is retained in the workspace `compiler-review/REVIEW.md` and its machine-readable probe results.

## Immediate repair tranche

1. Preserve borrowed-parameter validation failures; reject duplicate parameters; attach the offending statement's line to semantic errors. These shared fixes apply before every target.
2. Make target selection explicit for all four targets. Reject unimplemented Linux/macOS native output with a useful explanation. Keep frontend checking explicitly distinct from target qualification.
3. Reject incompatible DSP report flags instead of accepting and ignoring them; emit the requested DSP Release optimization report.
4. Stop IR emission after return/jump/branch; verify terminator position and successor shape.
5. Reject DSP reads not proven initialized on all incoming control-flow paths. This is a conservative interim compiler restriction, pending a normative scalar-initialization rule.
6. Add reproducible regressions and rerun host Debug/Release builds, existing C++ tests and native qualification probes. Update the current README while preserving historical records.

## Next correctness gates

- Implement one checked-int arithmetic contract for runtime and selector-time evaluation. Overflow currently wraps on both active targets. Fix +, -, *, division/remainder, negation, conversions and loop increments, preserving source diagnostics. A CPU trap alone is not a Shalimar diagnostic. Use a defined per-target runtime ABI and independently execute positive and negative boundary tests. Until then, do not claim runtime arithmetic conformance.
- Collect multiple independent semantic errors and required warnings; avoid producing cascades or mutating inferred AST declarations after a failed inference.
- Resolve canonical specification drift for trailing-zero returns, parenthesized call restrictions and scalar initialization without silently changing language semantics.
- Extend the verifier to typed operations, definition/dominance and call signatures. Add an effect/may-trap model before expanding optimization.
- Qualify Windows unwind metadata, function/parameter types, optimized variable locations and debugger stepping. Extend own DSP tools for DWARF or retain an explicit TI-only source-debugging contract.

## Native coverage gates

| Target | Current gap | Required work and validation |
|---|---|---|
| Windows x64 | Restricted patterns, no full language/runtime/unwind pipeline | General typed CFG selection; checked runtime; arrays/references/globals/text/printing; ABI and exception/debugger qualification |
| Linux x64 | Legacy emitter not integrated | System V ABI; ELF writer/relocations; runtime/startup; DWARF; native Linux emit-link-run and debugger tests |
| macOS ARM64 | Legacy emitter not integrated | Darwin ABI; ARM64 selection; Mach-O writer/relocations; runtime; DWARF; native ARM64 macOS execution/debugger tests |
| TMS6747 | Scalar subset, unchecked arithmetic and incomplete own-tool debugging | Checked runtime; remaining types/operations and multiple outputs; assembler/linker/simulator/CCS gates; scheduling after correctness |

Each language family must pass parse, check, IR, emit, link, runtime output/error, source Debug/Release and debugger gates separately. Unsupported features need explicit diagnostics. Native qualification requires the actual target/toolchain; frontend tests on Windows do not qualify Linux or macOS. The original 12 examples remain mandatory end-to-end completion gates.

The immediate tranche starts repair, not full compiler completion. Subsequent gates remain open until independently verified.

## First-tranche implementation status

Implemented parameter-error preservation, duplicate rejection and semantic line locations; explicit four-target selection and honest Linux/macOS output rejection; DSP report-option validation and optimization reports; return/branch/jump terminator enforcement; and DSP must-initialization analysis including joins and zero-trip loops.

Also replaced wrapping Windows selector constant arithmetic with checked signed evaluation and added shared conservative per-block known-overflow validation to both active backends. Known overflow is rejected at compilation. This is not a substitute for runtime-input overflow handling, which still wraps in fresh Windows/DSP execution probes.

The repeat review additionally found that integer constant propagation could substitute a real local initialized from an integer literal, changing later arithmetic types. Both local and cross-block passes now exclude declared non-integer symbols from integer facts. A real addition crossing INT_MAX remains real in Release and executes correctly in the DSP assembly VM. Broader typed-IR work remains necessary, especially for globals and value effects.

Current tests and evidence are summarized in `REV3_REPAIR_RESULTS.md`.
