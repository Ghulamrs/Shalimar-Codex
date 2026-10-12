# Shalimar Rev3 — developer handover

Prepared 12 October 2026 (Asia/Karachi). This describes the repaired workspace snapshot and recorded qualification, not a released or committed revision.

Snapshot identity: [source, test and documentation SHA-256 manifest](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/docs/HANDOVER_SOURCE_SHA256_2026-10-12.json). It excludes build outputs and vendor tool binaries. It identifies file contents; it does not certify that every file or feature was tested.

## 1. Start here

The compiler is a partially implemented native Shalimar compiler. Important correctness repairs, local integer/real vectors, and a restricted Windows debugger subset are implemented and tested. Full Shalimar 3.0 conformance, equivalent four-target coverage, complete debugging and broad optimization are not complete.

**Latest repaired project:**

```text
C:\Users\GRA\Documents\ChatGPT\Shalimar-ChatGPT\Rev3\Shalimar_TMS6747_Windows_Merge_Rev2\ShalimarCompiler
```

The directory's name contains `Windows_Merge_Rev2`, but its enclosing Rev3 snapshot is the current review/repair baseline. Do not replace it with the earlier workspace `ShalimarCompiler` folder, the similarly named original installation, or a ZIP solely on the strength of their names.

**Workspace:** `C:\Users\GRA\Documents\ChatGPT\Shalimar-ChatGPT`.

**Current host binaries:** `C:\Users\GRA\Documents\ChatGPT\Shalimar-ChatGPT\compiler-review\build\Debug\shalimarc.exe` and `C:\Users\GRA\Documents\ChatGPT\Shalimar-ChatGPT\compiler-review\build\Release\shalimarc.exe`.

Existing ZIPs are input/earlier snapshot artifacts and were not refreshed after these repairs. The original project at `C:\Users\GRA\Documents\Shalimar\ShalimarCompiler` was inspected, not repaired in place. The separate ASM6x/LNK6x/RTS6x/SIM6747 source projects were not modified.

The workspace Git repository has branch master with no commits, no configured remote and untracked project/evidence folders. There is no commit SHA identifying this work. Nothing was committed, pushed or submitted as a PR from this session. The last HTTPS check of `https://github.com/Ghulamrs/Shalimar-Codex.git` returned no refs; SSH failed host-key verification. Recheck the remote before importing or publishing anything. Do not assume it contains these changes.

## 2. Mandate and specification authority

The user's mandate was to critically review Rev3 for language coverage, four-target treatment, code quality, diagnostics, Debug/Release behavior and optimizations; repair confirmed defects autonomously; verify arrays against the intended language; and provide practical source/variable debugging comparable to C/C++ for Shalimar's own constructs. The intended larger solution combines three languages and four machines. The other two languages and their interoperability contract have not been specified in this work.

Authority used, in order:

1. The user's explicit decisions in this conversation, particularly the finalized array/buffer rules and mandatory debugger support.
2. [SHALIMAR_LANGUAGE.md](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/docs/SHALIMAR_LANGUAGE.md) — canonical Shalimar 3.0 syntax/semantics. On 12 October, its SHA-256 matched the original project's docs/SHALIMAR_LANGUAGE.md: `379029772F7FE6D6DCD4BB176353114EAF3378E09DB5FFE7D888576D162D6F8E`.
3. [ARRAY_CONTRACT_CLARIFICATIONS.md](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/docs/ARRAY_CONTRACT_CLARIFICATIONS.md) — approved clarifications that explicitly supersede conflicting older array examples. The extended decision record is [Array requirement decisions](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/compiler-review/ARRAY_REQUIREMENTS_DECISIONS.md).
4. [SPEC_FREEZE.md](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/docs/SPEC_FREEZE.md) — preserve approved semantics; distinguish frontend acceptance from implementation; reject unsupported native forms clearly. Interpreter implementation notes do not dictate the C++ compiler's architecture.
5. [Native return ABI](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/docs/TECHNICAL_MANUAL_RETURN_ABI.md), [Return argument rule](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/docs/RETURN_ARGUMENT_RULE.md) and [Trailing zero returns](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/docs/RETURN_TRAILING_ZERO_RULE.md) — inherited native return design and finalized frontend return rules. Read their qualification limits: DSP multi-result ABI remains unverified, and these documents do not establish all-target implementation.
6. [Debugger release requirements](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/docs/DEBUGGER_RELEASE_REQUIREMENTS.md) — debugger acceptance contract required for this revision.

Legacy backend/reference sources were inspected as implementation evidence, not treated as authority overriding user decisions. Their register/stack-slot indexes must not be mistaken for language array indexing. No new C++ language features were mandated. The debugging comparison means ordinary source debugging, values, scopes and call stacks for supported Shalimar constructs.

### Settled array and character-buffer rules

- Explicit extents are positive integer literals. Zero/negative sizes, local/global variable extents, and arithmetic sizes such as `[2+3]` are rejected.
- Empty brackets infer extents from a nonempty rectangular initializer. Explicit/inferred dimensions can mix when sizes agree. Without initialization, each unspecified dimension defaults to 3; an empty `{}` is ignored.
- Numeric initialization supplies the complete shape. Partial, oversized and ragged forms are errors. Comma-delimited omitted entries and trailing slots count toward shape and supply zero. Typed all-blank literals use the declared type; untyped all-blank literals default to int.
- Uninitialized arrays are forcibly zero-initialized. Array copies require identical element type, rank and dimensions and copy into existing storage. Row assignment preserves the row's exact shape.
- Indexes are zero-based int values checked against capacity. Indexed assignment checks the index before evaluating the right-hand-side value, including expressions that can trap or have effects.
- Text length excludes the zero terminator; capacity includes its storage. `char S[5] : "hi"` has length 2/capacity 5. `char S[] : "hi"` infers capacity 3. `char S[5] : "hover"` is an error because the terminator does not fit.
- String initialization writes content and one zero byte; slots beyond that terminator are unspecified. Without an initializer, all buffer slots are zero.
- `len(S)` counts text before the first zero; `S.row` reports capacity. All capacity indexes are writable, including the last slot. `S[4] = 'y'` is valid for capacity 5; an element write does not preserve or restore termination.
- The user allows garbage output after a buffer loses its terminator. Exact unterminated-buffer len/print behavior is not fully defined. Do not interpret this allowance as permission to claim arbitrary out-of-bounds memory reads are safe.

Numeric shape rules do not require a string's content length to equal buffer capacity. General cross-file imports, complete conversion/non-finite semantics and unresolved scalar initialization must be assessed against the canonical reference before adding policy.

## 3. What was inherited and what was repaired

The extracted baseline already contained a lexer/parser/checker, pooled AST handles, IR/optimizer, restricted Windows selection/COFF, legacy emitters, a DSP bridge/toolchain, return-layout design, tests and historical qualification. The independent DSP tools and earlier TI/CCS scalar-debugging evidence were inherited. The following are the repairs and additions carried out in this review sequence.

### Frontend, diagnostics and IR correctness

- Preserved borrowed-runtime-name parameter failures that had been detected then cleared.
- Rejected duplicate parameter names and added statement-line context to semantic failures.
- Reserved runtime helper names against source collisions.
- Normalized inferred/default array extents and ignored empty initializers; enforced literal-only positive extents, rectangularity, exact numeric shapes and literal string capacity.
- Corrected array shape lookup for indexed rows/copies and restored shape information across lexical blocks/functions without leaking parameter/scalar shadows.
- Stopped IR lowering after return/jump/branch and enforced terminator/successor structure through IR verification.
- Added index.check before lowering indexed assignment's value so bounds errors precede value-expression failures.
- Added DSP must-initialization data flow across reachable predecessors, joins and zero-trip loops. Reads not proven initialized are rejected. This is an interim backend restriction, not a newly approved universal scalar-zero rule.

### Target selection and optimization safety

- Added explicit windows-x86_64, linux-x86_64, macos-arm64 and tms6747 selections. Linux/macOS native emission rejects clearly; frontend-only checks explain their limits.
- Rejected incompatible DSP/Windows report options and emitted requested DSP Release optimization reports.
- Applied shared known-integer overflow validation and replaced wrapping Windows selector-time arithmetic with checked evaluation.
- Prevented integer propagation from substituting declared real/char/array values with integer facts. The real initializer regression `real x : 2147483647; return x+1` retains real arithmetic and produces 2147483648.0 in the qualified DSP path.
- Kept C++ assertions enabled in Release contract tests. Updated README status and marked older C-backend instructions historical.

### Runtime arithmetic and native numeric vectors

- Implemented runtime checked signed integer addition, subtraction and multiplication on Windows and DSP; DSP unary negation is also checked. Windows numeric-vector lowering now checks integer negation in that path. Do not infer universal operator coverage from this subset.
- Added a small Windows runtime diagnostic library with line-associated overflow/bounds messages, stderr output and exit status 1. Generated objects that reference its helpers must link that library and kernel32.lib.
- Added local rank-one integer vectors on both active targets: zero/literal/computed initialization, omitted slots, loads/stores, copies/self-copy, dimensions and checked indexing.
- Added local rank-one real vectors on both active targets: eight-byte elements, integer promotion, real +/−/*/negation, copies, signed zero, mixed numeric arrays and dimension queries.
- Preserved bounds-before-value failure ordering. Copies do not resize arrays or alias the source's storage.

### Windows debugging

- Added CodeView int/real array types, extents, actual frame offsets and initialized live ranges; exposed supported parameter/local locations.
- Replaced implicit frame-pointer location ranges with explicitly RBP-relative ranges.
- Corrected CodeView line-block sizes and collapsed adjacent operations of the same source line into one stop.
- Added .pdata/.xdata for the existing exact push-RBP/mov-RBP-RSP/sub-RSP-immediate prologue with a nonzero aligned frame. Both small and large allocation encodings are covered; metadata is emitted independently of requesting source symbols.
- Linked PDBs and used actual CDB source breakpoints, typed integer/real element inspection, local/parameter inspection, stack output, step-out and continued execution. This is executable debugger evidence, not just object-section inspection.

## 4. Current target matrix

| Target | Native execution | Debugger status | Main boundary |
| --- | --- | --- | --- |
| Windows x64 | Restricted native COFF/ABI patterns; local int/real vectors tested | CodeView/PDB/CDB subset tested; scoped live locations and one prologue family qualified | Array functions still single-block, without internal calls, at most one scalar int input and one int/real output |
| TMS6747, own tools | Assembly → ASM6x/LNK6x/RTS6x → linked .out; VM and SIM execution tested | Interactive source debugging unfinished; own assembler does not preserve required DWARF | Numeric-vector execution is broader than debugger coverage; arrays/references/globals/text and complete operators remain unequal |
| TMS6747, TI/CCS | Separate selectable TI pipeline inherited | Earlier 16 scalar source-debugger scenarios passed; latest array/arithmetic additions not freshly qualified in CCS | Retain as comparison/debugging path; do not transfer its result to own tools |
| Linux x64 | Frontend/IR selections; legacy emitter exists | No integrated qualified native debugger pipeline | ELF, System V ABI, runtime, native execution and DWARF integration needed |
| macOS ARM64 | Frontend/IR selections; legacy emitter exists | No integrated qualified native debugger pipeline | Mach-O, Darwin ABI, runtime, native execution and LLDB qualification needed |

Every vector is limited to 512 elements. Windows additionally limits the whole frame to 4000 bytes pending stack probes, so not every 512-element real vector fits. DSP limits slots to 4096. DSP uses eight-byte slots for both numeric types; integer elements occupy one word, real elements two. Windows int elements use four bytes and real elements eight, with suitable alignment. Do not export these implementation details as an array-parameter ABI without explicit design and tests.

Real scalar results use XMM0 on Windows and A5:A4 in the executed DSP subset. DSP multi-output support and its indirect-result ABI are still open. Windows structured returns exist only in restricted patterns. Native vector initialization/copy is unrolled; no target-independent performance claim is made.

## 5. Recorded qualification and its limits

These are existing test results verified during preparation of this handover. The document task did not rerun every suite. Host Debug/Release is the compiler executable configuration; source --debug/--release controls the compiled Shalimar program. They are different axes.

| Check | Result | Evidence |
| --- | --- | --- |
| Array frontend acceptance/rejection | 144/144 conforms | [Array contract results](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/compiler-review/array-contract/results.json) |
| Checked arithmetic boundary suites | 48/48 per host configuration, 96 total | [Debug-host arithmetic](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/compiler-review/arithmetic-debug/results.json) and [Release-host arithmetic](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/compiler-review/arithmetic-release/results.json) |
| Local integer arrays | 192/192 | [Integer-array results](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/compiler-review/native-int-arrays/results-all.json) |
| Local real arrays | 248/248 | [Real-array results](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/compiler-review/native-real-arrays/results-all.json) |
| Windows/DSP existing native checks | Windows 108/108; DSP 52/52 | [Native regression results](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/compiler-review/repair-native/results.json) |
| CDB mixed numeric-vector scenarios | 8/8 | [CDB results](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/compiler-review/debugger/results.json) |
| Windows RtlVirtualUnwind | 60 focused checks across configurations and mixed/small/large frames | [Windows debugger qualification](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/docs/WINDOWS_DEBUGGER_QUALIFICATION.md) and debugger runner |
| C++ contracts | 14/14 Debug; 14/14 Release | compiler-review/repair-tests CTest outputs |
| Earlier TI/CCS | 16 scalar-debugger and 48 runtime cases in inherited report | [DSP checkpoint](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/docs/TMS6747_REV3_QUALIFICATION.md); not a current rerun |

The unwind result tests caller RSP/RBP/return PC at entry, partial prologue, body and additional temporary allocation. Per-input debugger result rows reference the same checks; summing them again would double-count.

Windows real runtime results are compared byte-for-byte as binary64. VM6747 checks both DSP result words exactly. SIM6747 checks linked execution, full integer status, termination and diagnostics; its real-result wrapper status is zero and does not independently prove exact floating-point values. Do not call VM and SIM equivalent validators. CDB decimal displays establish debugger inspection, not bitwise numeric correctness.

The 154-fixture audit is a selected mixed corpus with intentionally invalid and historical programs, not a conformance percentage. Initial results were frontend 132, Windows 27, DSP 21; the last recorded post-integer-array corpus was frontend 116, Windows 28, DSP 22. The frontend reduction includes deliberate rejection of now-forbidden partial/ragged/truncated/variable-size examples. Do not mistake a lower acceptance count for automatic regression. A full corpus was not rerun after the final real-array addition. The last original-12 gate remained frontend 12/12 and native emission 0/12 on both active targets; those examples are still an essential end-to-end release gate.

## 6. Remaining deficiencies

### Language implementation

Char-buffer native storage, string initialization/assignment, len behavior and text output; multidimensional arrays/rows; globals and initialization; array/reference parameters; general calls/control flow interacting with arrays; indexed compound assignment; complete operators, conversions and library/printing constructs remain open. Native support differs by target. A parsed or checked construct must not be marked implemented until emitted, linked and executed.

Audit signed division/remainder including zero and INT_MIN/−1, conversions, powers, loop increments and all arithmetic paths. Existing historical AST-to-C support is not evidence for the current native driver. Scalar default initialization remains a specific specification/implementation question; current rejection must not silently become a different language rule.

### Diagnostics and internal design

The compiler is still largely fail-fast, despite the reference's expectation of multiple independent checker diagnostics. Rich warnings, consistent source columns, backend capability diagnostics and diagnostic parity need work. IR verification needs complete types, definition/dominance and call-signature checks. Lowering still relies substantially on operation/operand spellings and source names; stable symbol identities and scope/lifetime metadata are needed before accepting shadowed locals in general code generation/debugging.

### Debugging

Windows needs lexical scopes/shadowing, columns, complete function/parameter types, other array forms, all prologue/epilogue families and optimized location changes. One RBP-frame family is qualified, not every function. Global/array-reference inspection and full source step-in/over scenarios remain to be established.

DSP arrays lack qualified DWARF type/location descriptions. Own-tool --debug currently means unoptimized IR, not source-debuggable output. Explicit own-tool --debug-symbols is rejected. SIM6747 has no implemented interactive Shalimar debugging session/controller/adapter. Linux/macOS require their native pipelines before debugger completion can be claimed.

### Optimization, quality and release engineering

Release contains IR folding/propagation, branch simplification and narrow loop/strength-reduction work; it is not a complete optimization pipeline. Maintain an explicit side-effect/may-trap model, preserve checks and variable locations, replace unrolled array operations where measured and implement DSP scheduling only after correctness. SIMD/VLIW, cache/memory timing parity and broad performance superiority are not qualified.

MSVC builds use C++98-compatible source/CMake settings, but MSVC compatibility mode is not an independent strict-C++98 proof. Sanitizer/fuzzing coverage, portability checks, exhaustive ABI testing, large-stack probing/limits and current four-target CI remain incomplete. Test scripts contain local Microsoft/SDK paths and need configuration on another machine. Release packaging, commits, remote history and reproducible clean checkout are not yet established.

## 7. Source map for the incoming developer

| File | Responsibility |
| --- | --- |
| [src/Parser.cpp](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/src/Parser.cpp) | Declaration brackets, literals and parsing |
| [src/SemanticChecker.cpp](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/src/SemanticChecker.cpp) | Parameter validation, shapes/types, capacity and rule checks |
| [src/IRBuilder.cpp](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/src/IRBuilder.cpp) | Array operations, index.check ordering and terminators |
| [src/IRVerifier.cpp](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/src/IRVerifier.cpp) | IR structural checks |
| [src/IROptimizer.cpp](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/src/IROptimizer.cpp) | Facts/folding/CFG changes and trap-sensitive transformations |
| [include/IR.h](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/include/IR.h) | Operand, source-location, signature and return-layout contracts |
| [include/backend/common/CheckedInteger.h](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/include/backend/common/CheckedInteger.h) | Shared integer checks |
| [src/backend/x86_64/ArraySelection.h](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/src/backend/x86_64/ArraySelection.h) | Windows numeric-vector allocation, lowering and live locations |
| [src/backend/x86_64/IRSelector.cpp](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/src/backend/x86_64/IRSelector.cpp) | Other restricted Windows patterns and vector dispatch |
| [src/backend/x86_64/InstructionEncoder.cpp](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/src/backend/x86_64/InstructionEncoder.cpp) | Machine bytes, array accesses, conversions and runtime helper calls |
| [src/backend/coff/CoffWriter.cpp](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/src/backend/coff/CoffWriter.cpp) | COFF symbols/relocations, CodeView and unwind sections |
| [include/backend/coff/CoffWriter.h](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/include/backend/coff/CoffWriter.h) | Function/debug-variable metadata passed to the writer |
| [src/backend/tms6747/IRAssembly.cpp](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/src/backend/tms6747/IRAssembly.cpp) | DSP slots, numeric arrays, data flow and checked code emission |
| [src/backend/legacy_port/Tms6747.cpp](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/src/backend/legacy_port/Tms6747.cpp) | DSP instruction spellings, slot storage, function/debug emission |
| [src/backend/tms6747/Toolchain.cpp](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/src/backend/tms6747/Toolchain.cpp) | Own/TI tool invocation, startup/link pipeline |
| [src/main.cpp](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/src/main.cpp) | Target dispatch, reports, helpers and qualified unwind-prologue metadata |
| [runtime/NativeRuntime.cpp](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/runtime/NativeRuntime.cpp) | Windows fatal diagnostics and overflow/bounds helper entry points |
| [tests/run-native-real-arrays.py](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/tests/run-native-real-arrays.py) | New numeric coverage regression runner |
| [tests/run-windows-array-debugger.py](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/tests/run-windows-array-debugger.py) | CDB and RtlVirtualUnwind acceptance checks |

Compiler flow: lex/parse → common semantic checking → IR lowering/verification → optional source Release IR optimization → target selection → native object/assembly → toolchain linking/runtime. Add typed/layout/scoping metadata at shared stages; serialize target-specific debug formats after code generation/optimization so locations match final instructions.

The numeric-vector selector is an included helper inside IRSelector.cpp's namespace, not an independently compiled translation unit. Add encoder methods to both header and implementation. Tests/driver receive locations through coff::FunctionBytes/DebugVariable. Do not introduce scalar types or fabricated offsets for unavailable array variables.

## 8. Tooling and reproduction

Use a fresh CMake directory if the project moves; cached paths are absolute. From the workspace, in PowerShell:

```powershell
$projectRoot = 'C:\Users\GRA\Documents\ChatGPT\Shalimar-ChatGPT\Rev3\Shalimar_TMS6747_Windows_Merge_Rev2\ShalimarCompiler'
$taskBuild = 'C:\Users\GRA\Documents\ChatGPT\Shalimar-ChatGPT\compiler-review\build'
$taskTests = 'C:\Users\GRA\Documents\ChatGPT\Shalimar-ChatGPT\compiler-review\repair-tests'
$cmakeExe = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$ctestExe = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe'
& $cmakeExe -S $projectRoot -B $taskBuild -G 'Visual Studio 17 2022' -A x64
& $cmakeExe --build $taskBuild --config Debug
& $cmakeExe --build $taskBuild --config Release
& $cmakeExe -S "$projectRoot/tests/windows-qualification" -B $taskTests -G 'Visual Studio 17 2022' -A x64
& $cmakeExe --build $taskTests --config Debug
& $cmakeExe --build $taskTests --config Release
& $ctestExe --test-dir $taskTests -C Debug --output-on-failure
& $ctestExe --test-dir $taskTests -C Release --output-on-failure
python "$projectRoot/tests/run-native-int-arrays.py" --compiler-build $taskBuild --output 'compiler-review/recheck-int'
python "$projectRoot/tests/run-native-real-arrays.py" --compiler-build $taskBuild --output 'compiler-review/recheck-real'
python "$projectRoot/tests/run-windows-array-debugger.py" --compiler-build $taskBuild --output 'compiler-review/recheck-debugger'
python "$projectRoot/tests/run-native-arithmetic.py" --compiler-build $taskBuild --configuration Debug --output 'compiler-review/recheck-arithmetic-debug'
python "$projectRoot/tests/run-native-arithmetic.py" --compiler-build $taskBuild --configuration Release --output 'compiler-review/recheck-arithmetic-release'
```

Also use workspace compiler-review/verify-array-contract.py, verify-repairs-native.py and audit.py for frontend, supplied native corpus and selected broad-stage evidence. These workspace scripts are not all copied into the compiler project; preserve them with the handover. Inspect argument handling before changing output paths. Do not overwrite original evidence when rechecking.

Windows tools: MSVC 14.44.35207 Hostx64/x64 linker/dumpbin, Windows SDK 10.0.26100.0 kernel32.lib, LLVM under C:\Program Files\LLVM\bin, CDB under C:\Program Files (x86)\Windows Kits\10\Debuggers\x64, and Python 3.12. The runtime target disables /GS and default CRT linkage and avoids Debug RTC dependencies. Link generated checked objects with shalimar_runtime.lib and kernel32.lib; selected sqrt corpus probes use a test shim, not a complete production math runtime.

Bundled own DSP binaries/runtime are [Own DSP toolset](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/toolsets/shalimar-c6747/README.md). Their manifest records upstream paths/hashes. Originals are C:\Users\GRA\source; source projects are under C:\Users\GRA\Developer\Claude\M10. TI C6000 7.4.4 is installed under C:\ti\ccsv5\tools\compiler\c6000_7.4.4; CCS 5.5 is the inherited source-debugging comparison environment. No TI tools are bundled with the own-tool set.

The driver supports --target, --toolchain=shalimar|ti, --tools-root, --rts-lib, --dsp-entry and --debug/--release as documented. Own DSP selection is separate from TI. VM6747 interprets assembly; SIM6747 --run executes linked machine code. Do not debug the Windows simulator executable and confuse its C++ locals with simulated Shalimar variables.

## 9. Next mandates and acceptance gates

1. **Version and preserve the repaired snapshot.** Inventory source/evidence, establish a clean Git import and ignores, retain the evidence externally as needed, and publish only under explicit repository authorization. Do not blindly add build outputs, vendor tools or old ZIPs.
2. **Implement native char buffers next.** Use the settled rules above without reopening agreed questions. Verify zero initialization, literal/inferred capacity, exact-fit errors, one terminator, unused tail semantics, all capacity-index writes, text length/capacity, copies and error ordering. Add Windows/DSP execution and debugger fixtures.
3. **Complete multidimensional arrays, globals and array/reference arguments.** Define layout and bounds/shape metadata centrally; preserve caller storage and lifetime; handle row copies with exact shapes and provide debugger dimensions.
4. **Generalize Windows selection for CFG/calls and stable symbol identities.** Add branches/loops, scoped/shadowed locals and full signatures incrementally. Validate recursion, nested calls, temporary liveness, overflow and source stepping.
5. **Implement SIM6747 address-level debugging, then source debugging.** Read the source project before copying it into a writable development checkout; the original directory is outside this session's permitted write roots. Add paused/running/exited/faulted states, address breakpoints and resume suppression, execute-packet stepping and read-only registers/memory. Preserve CIO/exit hooks and stop inside software-pipelined loop execution. Inspection must not alter the resumed result. Then preserve/read DSP debug metadata, add source stepping/stack/variables and a Windows debugger adapter.
6. **Close diagnostics/arithmetic/type gates.** Complete multi-error reporting, typed IR/call validation and the unchecked/unimplemented operations listed above; document unresolved semantics precisely rather than silently weakening the language.
7. **Bring up Linux and macOS native pipelines.** Implement the actual object formats/ABIs/runtime and qualify execute/debug behavior on the corresponding hosts. Neither native host was available here; Windows frontend results cannot substitute.
8. **Optimize and package after correctness.** Test effects/traps, stack limits and Debug/Release invariants, measure code size/performance per target, then close all original examples, release documentation and mixed-language ABI requirements.

Debugging should stop/resume the simulated DSP, not require physical hardware for the simulator route. Step into/over/out belong to the debugger layer coordinated with simulator control; stack up/down selects frames without execution. Existing SIM6747 Cpu6x exposes step(), beforeIssue, register/control/memory access and pending-result views, but its host loop ends on a false step result and its loop-buffer path bypasses the ordinary beforeIssue hook. These are concrete integration points. Do not blindly drain or flush the pipeline on every stop; define halt/inspection semantics and test uninterrupted execution against pause/inspect/resume, including delayed branches, loads/stores, CIO, loops and faults.

Every mandate closes only with explicit acceptance evidence: parse/check → typed IR → emit → link → execute/error behavior → source Debug/Release → debugger where promised. Keep target/toolchain results separate. The existing [Language coverage work queue](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/docs/LANGUAGE_COVERAGE_WORK_QUEUE.md) and [Debugger release requirements](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/docs/DEBUGGER_RELEASE_REQUIREMENTS.md) remain the active work list and release gate.

## 10. What this handover does and does not establish

This handover identifies the latest source, approved decisions, repair history, reproducible evidence, tooling, current defects and next mandates. It also identifies inherited results and documents whose statements are now stale. It does not certify full conformance, full simulator/hardware fidelity, universal ABI compatibility, C/C++ feature parity, production-ready math/runtime facilities, all-target optimization or release readiness. It does not contain the complete external simulator/toolchain source projects or represent a committed Git revision.

Several historical documents remain intentionally preserved: REVIEW.md, REV3_REPAIR_PLAN.md, REV3_REPAIR_RESULTS.md, SPEC_QUESTIONS.md, TMS6747_REV3_QUALIFICATION.md, HANDOFF.md and older README sections describe checkpoints with now-obsolete restrictions. In particular, first-tranche claims that runtime +/−/* overflow still wraps and that all arrays are unsupported are superseded by current runtime/vector evidence. SPEC_QUESTIONS.md references older AST-to-C behavior; its claimed division/conversion/multi-output coverage must not be transferred to the native driver. Consult this handover and the current numeric-array/Windows-debugger qualification reports first, then inspect the code and rerun the relevant tests.
