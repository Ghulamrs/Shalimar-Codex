# Windows compiler qualification — 11 October 2026

**Verdict: NOT qualified as a fully conforming Shalimar 3.0 compiler.**

The compiler builds and executes a restricted Windows x64 native subset. It cannot compile the complete supplied positive program corpus to native code, and a supported real subtraction expression produces an incorrect runtime result. Successful frontend checking must not be represented as successful compilation and execution.

## Scope and method

Assessed the workspace copy of `C:\Users\GRA\source\ShalimarCompiler`, using Visual Studio Community 2022/MSVC 19.44 and the Windows x64 target. No compiler source or supplied fixtures were changed. The separately mentioned Rev54 PARTIAL ZIP was not integrated.

Inventoried every `.shl` and `.shm` file under `examples/` and `tests/`: **155 files**, comprising one example, 130 `.shl` tests and 24 `.shm` tests. The `.shm` files include both the 12 original programs and the 12 specification-example copies; counts include those copies. Recorded SHA256 hashes for every input and the language reference.

For each file, ran `--check`, `--dump-ir`, and `--object`, crossing both compiler executable builds (Debug/Release) with both source compilation modes (`--debug`/`--release`). This gives **1,860 corpus invocations**, plus 44 invocations of the 11 explicit normative negative cases. All diagnostics and emitted objects are retained under `build/vs2022/qualification/`.

Every successfully emitted object was inspected and linked into its own Windows DLL using Microsoft link.exe. Every linked DLL was executed in an isolated Windows Python process using ctypes: its no-output main was called and its supported result-producing exports were tested against explicit expected numeric values. Structured outputs were tested through the compiler's documented caller-owned result-buffer ABI. Test buffers were initially filled with nonzero bytes to test actual zero initialization. Each DLL invocation is isolated so a crash would be recorded as a failed process. DLL loading and no-output main calls are smoke checks, not full language coverage.

The sqrt case uses the independent Microsoft MASM test shim, not a production CRT import. These DLLs use `/noentry /nodefaultlib`; this does not qualify production executable startup or the complete runtime library.

## Results

Both compiler executable builds produced the same stage outcomes for every input. The following counts apply independently to each build:

| Stage | Source Debug mode | Source Release mode |
| --- | ---: | ---: |
| Supplied files completing frontend/IR check | 133/155 | 129/155 |
| Supplied files completing IR dump | 133/155 | 129/155 |
| Supplied files generating native COFF objects | 26/155 | 26/155 |
| Emitted objects linking as test DLLs | 26/26 | 26/26 |
| Emitted-source runtime cases meeting tested expectations | 25/26 | 25/26 |
| Explicit normative negative cases correctly rejected | 11/11 | 11/11 |
| 12 specification examples completing frontend check | 12/12 | 12/12 |
| 12 specification examples generating native objects | 0/12 | 0/12 |

Across the four build/mode combinations, all **104** emitted objects link, and **100/104** emitted-source runtime cases meet their expectations. There are **136** explicit exported-result assertions; **132 pass and 4 fail**, the latter all reproducing the same subtraction defect. The rev33 fixture contains only an empty main and has smoke execution coverage only. The tests use representative inputs, not exhaustive arithmetic/domain checks.

All 12 supplied C++ implementation test sources were also compiled with MSVC and attempted in both configurations, with `assert` enabled even in Release: **11/12 build and pass in each configuration** (22/24 aggregate). `arena_lifetime.cpp` does not compile: line 17 calls `AstRef<LiteralExpr>::get()`, which no longer exists. This is a stale implementation test, not evidence that the compiler rejected a Shalimar program.

## Blocking findings

### Incorrect real subtraction

`tests/rev49_real_parameter_subtract.shl` defines `subtractOffset(x)` as `x - 2.25`. Passing `1.5` returns **3.75**, while the expected result is **-0.75**. This happens with both compiler executable builds and both source modes.

The selector's earlier real arithmetic branch accepts both `binary +` and `binary -` but emits an addition instruction. It precedes the separate subtraction branch, so the subtraction branch does not handle this fixture. See `src/backend/x86_64/IRSelector.cpp`, approximately lines 604–675. The saved runtime results reproduce the defect without relying solely on instruction-text inspection.

### Release optimization produces invalid IR

These inputs complete Debug-mode checking but fail Release-mode checking and native generation:

| Input | Release diagnostic |
| --- | --- |
| examples/hello.shl | optimized IR verification: noncanonical block ID in main |
| tests/ir_control_flow.shl | optimized IR verification: noncanonical block ID in main |
| tests/ir_cross_block.shl | optimized IR verification: invalid successor bb3 in main |
| tests/ir_optimization.shl | optimized IR verification: invalid successor bb3 in main |

The verifier rejects the invalid IR; this is a detected optimizer defect, not a runtime result or successful Release compilation.

### Native language coverage is incomplete

None of the 12 specification example programs, including factorial, Fibonacci, GCD, matrix inversion and quadratic roots, generates a Windows native object. Diagnostics identify unsupported function/control-flow forms and unsupported global initializers. Printing, arrays/strings, general control flow, globals, general signatures and library/runtime integration are not qualified by the restricted objects that do compile. The full corpus results retain each precise diagnostic.

### Expected rejection and historical fixtures

The 22 Debug-mode frontend rejections are not all compiler defects:

- 18 are intentionally invalid fixtures, corroborated by their regression scripts or return-rule documents: malformed/oversized initializers, undefined variables, later-global references, invalid borrow names/arity, invalid precision calls, invalid references, excess return outputs and nested return calls.
- Three old positive fixtures (`string_return`, `ir_native_global_string_initializers`, `rev14_char_return_contract`) return arrays, prohibited by the 3.0 reference. Their rejection is consistent with that rule; their historic success expectations are obsolete.
- `array_parameter_dimensions.shl` contains calls inside parenthesized return expressions. It conflicts with the later documented return-call prohibition and needs reconciliation as a fixture/specification issue.

Runtime-negative fixtures such as overflow and out-of-bounds inputs are not counted as compile-time rejection tests merely because their names suggest errors. Most do not reach native generation, so their expected runtime diagnostics remain unqualified.

## Specification and debug qualification limits

Used `docs/SHALIMAR_LANGUAGE.md` as the 3.0 baseline, while recording later return-rule documents as development-history evidence. There is specification drift: section 7.4 says returned value counts must match, while `RETURN_TRAILING_ZERO_RULE.md` permits omitted trailing values and describes zero initialization. The prohibition of calls inside return parentheses is also recorded separately. These documents need reconciliation before a precise language-version conformance claim; the assessment does not silently rewrite the baseline or adopt document instructions as a new user request.

Fresh symbol probes show the supplied source can emit a `.debug$S` CodeView section and Microsoft link.exe generates a nonempty PDB for the simple `rev54_codeview_probe.shl` in all four build/mode combinations. This corrects older documentation that describes all CodeView/PDB generation as absent. Actual Visual Studio source stepping, variable locations, stack unwinding and complete generated-program debug coverage were **not verified**.

The complete historical shell suites were not claimed as passing: several invoke the removed `--emit-c`/`--ir-native` interface and encode outdated language expectations. Their supplied Shalimar inputs were all covered by this native corpus assessment, and all supplied C++ tests were attempted separately. Frontend acceptance and a finite test corpus do not prove all grammar/type/semantic rules of the full reference.

## Evidence and reproduction

Machine-readable evidence under `build/vs2022/qualification/`:

- `corpus-results.json` and `corpus-results.csv`: every stage outcome, diagnostic, source hash and emitted-object path.
- `link-results.json`: every emitted-object link and exported symbol list.
- `native-runtime-results.json`: numeric expected/actual values, process exit codes and runtime failures.
- `cpp-results.json`: every implementation test build/run result.
- `symbols-results.json`: CodeView section and PDB generation probes.

From the compiler workspace copy:

```powershell
.\build-windows.ps1
.\tests\run-vs2022-windows.ps1
python .\tests\conformance\run_windows_corpus.py
python .\tests\conformance\link_windows_corpus.py
python .\tests\conformance\run_windows_native_corpus.py
python .\tests\conformance\probe_windows_symbols.py
```

For the C++ tests, open a Visual Studio 2022 x64 Native Tools command prompt and run:

```bat
python tests\conformance\run_windows_cpp_tests.py
```

Link/probe scripts currently use this machine's verified MSVC 14.44.35207 tool paths. Python must be 64-bit for the x64 DLL runtime tests. Read the JSON results for the verdict; successful completion of a report-producing script does not mean all tests passed.

## Qualification decision

**Shalimar partial compiler with restricted Windows x64 native code generation: demonstrated.**

**Full Shalimar 3.0 language conformance or production Windows compiler qualification: failed.**

Qualification requires correcting arithmetic and optimizer defects, bringing valid specification examples through native generation/link/execution with expected outputs and diagnostics, implementing the missing language/runtime paths, reconciling specification amendments and fixture expectations, and verifying the promised debugger capabilities. Compiler-source build success alone does not meet those requirements.
