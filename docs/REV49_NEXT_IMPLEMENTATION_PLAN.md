# Shalimar rev49 — next implementation plan (11 October 2026)

## Baseline and limitations

This is a development snapshot, **not** a completed release. The latest retained P0 working tree compiles with the existing C++98 build. A source scan finds 276 occurrences of `AstRef<` and 51 occurrences of `astCast<` in `include/` and `src/`. The user has deferred their removal; do not claim they are eliminated. The retained conformance baseline documented 24 passes and 28 open gaps; rerun before making any new claim.

## Priority 1: Native multiple returns (Windows x86-64)

Connect typed IR return layouts to actual caller-owned result storage. Implement the ABI hidden-result pointer, callee writes, caller reads, mixed `int`/`real`/`char` results, alignment, and tests. Preserve single-result ABI register returns. Test object disassembly and, where possible, link and execute on Windows. No array-valued returns.

## Priority 2: Native language conformance

Close remaining IR-selector and backend gaps against the normative language specification, beginning with runtime arithmetic between two parameters, local variables, assignments, conditionals, loops, calls, strings, globals, arrays, and references. Each capability must have positive/negative semantic tests and native output checks. Fix compiler behavior, not tests merely to make them pass.

## Priority 3: Debug and Release

Emit real target-compatible debug information, not just custom source maps; verify with target debug tooling. Verify Release optimization equivalence against Debug, including the optimization loop before assembly/object generation. Exercise executable linking and runtime tests.

## Priority 4: Native target verification

P0 Windows x86-64 COFF/CodeView/PDB; P1 Linux x86-64 ELF/DWARF; P2 macOS ARM64 Mach-O/DWARF; P3 TI TMS6747 DSP. Maintain native Shalimar IR and target-specific code generation; do not silently substitute an AST-to-C fallback. Mark any target unverified until built, linked, and executed in its target environment.

## Deferred architecture cleanup

Return to eliminating `AstRef<T>` and `astCast<T>` by coordinated conversion of AST child fields, parser, semantic checker and IR builder to named IDs with checked arena access. Keep standard C++98 STL templates. Avoid explicit `new`/`delete` and `malloc`/`free` in project source. Do not claim memory safety from grep alone.

## Release gate

Complete normative feature audit; strict C++98 build; regression and optimization equivalence tests; native object/link/run and debug checks per target; accurate documentation and full source/test/IDE deliverables. Do not label rev49 final until all acceptance criteria are met.
