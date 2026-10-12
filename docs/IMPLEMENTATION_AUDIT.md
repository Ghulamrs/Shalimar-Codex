> rev6 update: this historical document must be read with [REV6_TEST_RESULTS.md](REV6_TEST_RESULTS.md), which is authoritative for current tested status and limitations.

# Shalimar Implementation Audit and Completion Checklist

**Audit baseline:** 2026-10-11  
**Scope:** the current repository source, regression fixtures, build definitions, and existing project status notes.

This audit is a working checklist, not a declaration that Shalimar 3.0 is complete. The repository README explicitly describes the compiler as a partial implementation. Each check below must be backed by a repeatable test or a recorded native build before it can be marked complete.

## Verified baseline

- [x] C++98 build configuration is enforced by CMake.
- [x] The IR implementation is split by role into `IRBuilder.cpp`, `IROptimizer.cpp`, and `IRCodeGenerator.cpp`.
- [x] The Linux smoke suite passed during this audit using `sh tests/run-smoke.sh`.
- [x] The suite exercises AST-to-C and selected IR-native cases, generated-C execution, runtime bounds failures, string handling, references, and object generation.
- [x] The language reference is identified as the intended Shalimar 3.0 semantic baseline; implementation-specific notes are distinguished from compiler architecture.
- [x] A role-by-role feature-parity matrix has been added in `docs/FEATURE_PARITY.md`.
- [x] Specification freeze policy and unresolved semantic questions are recorded in `docs/SPEC_FREEZE.md` and `docs/SPEC_QUESTIONS.md`.
- [x] Expression regression coverage now checks precedence, right-associative power, unary minus, integer/real conversions, and AST/IR output parity.
- [x] A clean, isolated Release configuration was built with strict C++98 warnings-as-errors; a Linux x86-64 IR-native object was generated and its ELF architecture verified.
- [ ] Windows/MSVC 2022 native build and run are not independently verified here.
- [ ] macOS/Xcode 26+ native build and run are not independently verified here.
- [ ] Full conformance to every rule in `docs/SHALIMAR_LANGUAGE.md` has not been established.

## Language and backend completion gates

- [x] Build a source-derived feature matrix for the lexer, parser, semantic checker, AST-to-C backend, and IR-native backend.
- [x] Mark major specification features as tested subset, partial, rejected/unsupported, or unverified in the feature matrix.
- [x] Add a targeted expression parity fixture for precedence, power, unary minus, and ordinary conversions; integer-boundary, exceptional-real, and malformed-expression testing remains open.
- [ ] Verify all supported array ranks, initializers, indexing, dimensions, references, and runtime bounds behavior.
- [ ] Verify strings, concatenation, append, return values, global initialization, and ownership/lifetime across both backends.
- [ ] Verify Release output against Debug output for the optimizer's supported transformations.
- [ ] Verify diagnostics preserve source path and line information for lexer, parser, semantic, and backend errors.
- [ ] Produce native build/run evidence for Windows, Linux, and macOS before claiming all three targets are supported.
- [ ] Write the final express language reference only after the implementation matrix is reconciled with observed behavior.

## Release acceptance criteria

1. The strict C++98 build succeeds from a clean build directory.
2. The full smoke suite succeeds.
3. Every supported construct has positive tests; important invalid constructs have negative tests.
4. AST-to-C and IR-native output agrees for each shared supported test case.
5. Debug and Release behavior agrees for programs covered by optimization tests.
6. Native platform support is stated only for toolchains that were actually built and tested.
7. The release archive includes source, tests, documentation, project files, and any prebuilt binaries clearly labeled by target.
8. A fresh extraction of the archive can be built using the documented commands.

## Known qualification

The smoke suite passes but the default AST backend's generated C emits some unused-static-helper warnings. These are currently non-fatal. The IR-native generated-C checks use stricter warning handling. The compiler's full memory-safety properties have not yet been proven by sanitizers or a comprehensive ownership review.
