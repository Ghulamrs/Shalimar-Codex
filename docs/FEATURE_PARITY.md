> rev6 update: this historical document must be read with [REV6_TEST_RESULTS.md](REV6_TEST_RESULTS.md), which is authoritative for current tested status and limitations.

# Shalimar Compiler Feature-Parity Matrix

**Audit date:** 2026-10-11  
**Language target:** `docs/SHALIMAR_LANGUAGE.md` (Shalimar 3.0)  
**Evidence:** compiler source, `tests/run-smoke.sh`, and the regression fixtures in `tests/`.

This matrix distinguishes language semantics from backend implementation. “Tested” means a regression fixture currently exercises the feature; it does not imply every edge case in the language reference is covered. The AST-to-C backend is the default. `--ir-native` is experimental and must reject unsupported constructs rather than silently falling back.

## Status key

- **Tested subset** — source and regression coverage demonstrate the described subset.
- **Partial** — some supported forms are tested, but the language rule or edge cases are incomplete.
- **Rejected/unsupported** — the compiler or backend rejects it, or it is not implemented.
- **Unverified** — project files exist, but native toolchain execution has not been demonstrated.

## Frontend and language constructs

| Feature | AST-to-C backend | IR-native backend | Evidence / remaining gap |
|---|---|---|---|
| C++98 lexer/parser/checker | Tested subset | Shared frontend | Strict C++98 build and smoke suite pass; complete Shalimar 3.0 conformance is not established. |
| `int`, `real`, `char` scalars | Tested subset | Tested subset | Scalar declarations, assignments, calls, printing, comparisons and conversions are covered in the supported subset. Integer and conversion checks cover common expression paths; loop updates, array-dimension arithmetic, and real non-finite behavior remain open. |
| Integer/real literals and unary minus | Tested subset | Tested subset | `expression_precedence.shl` checks unary-minus precedence and integer/real output. Boundary/error semantics still need dedicated tests. |
| Arithmetic `+ - * / %` | Tested subset | Tested subset | Scalar integer arithmetic, unary negation, division/remainder by zero, and signed minimum divided by -1 are runtime-checked. Loop-update and array-dimension arithmetic still need audit; real arithmetic error semantics remain open. |
| Power `^` | Tested subset | Tested subset | Emits `pow`; expression test checks right associativity and unary-minus binding. Real-domain and non-finite behavior remain open. |
| Comparisons and logical `&`, `|` | Tested subset | Tested subset | Numeric comparisons and string-content comparisons are supported; exhaustive short-circuit/side-effect semantics need dedicated coverage. |
| `int(x)`, `real(x)`, `char(x)` | Tested subset with range checks | Tested subset with range checks | `int` and `char` conversions check finite/range conditions before narrowing; `real` conversion is a cast. Endpoint coverage and every generated path still need audit. |
| Function declarations/calls | Tested subset | Tested subset | Scalar calls and `char[]` returns work. IR-native does not yet support multi-output functions. |
| Multi-output functions and multi-assignment | Tested subset | Rejected/unsupported | AST fixtures cover multi-output returns/assignment; IR-native supports at most one return value. |
| Scalar reference parameters (`&`) | Tested subset | Tested subset | Copy-in/copy-back tests include aliases and indexed `int`/`char` elements. Computed reference arguments are rejected. |
| `if`/`else`, `while`, `for`, `break`, `continue`, `return` | Tested subset | Partial | Common branches and loops work; IR-native supports only the control-flow subset it validates. |
| `uses` built-ins | Partial | Restricted | AST-to-C resolves the documented borrowed built-in subset in one file; IR-native currently allows only `len`. General cross-file imports are not implemented. |
| `?`, `??`, `prec(n)` | Tested subset | Tested subset | Scalar/string output and precision tests exist. Printing all array shapes as formatted grids is incomplete. |
| Parser/checker diagnostics | Partial | Partial | Invalid syntax and unsupported IR constructs produce diagnostics, but not every specified diagnostic or error-collection rule is implemented. |

## Arrays and strings

| Feature | AST-to-C backend | IR-native backend | Evidence / remaining gap |
|---|---|---|---|
| Fixed-size local arrays | Tested subset | Tested subset for rank 1–2 `int`, rank 1 `char` | Bounds, initializers, zero-fill, reassignment, and attributes have fixtures. IR-native rank 3+ and other array element types are unsupported. |
| Runtime-sized local arrays | Tested subset | Rejected/unsupported | AST backend has allocation, dimension and bounds checks; runtime-sized arrays are not in the IR-native subset. |
| Fixed-size global `int` arrays | Tested subset | Tested subset for rank 1–2 | Global initialization, mutation, attributes, forwarding and reassignment are tested. |
| Global `real`/`char` scalar values | Tested subset | Tested subset | Source-order initialization and cross-function reads/writes are covered for supported scalar globals. |
| Global `char[]` strings | Tested subset with known assignment caveats | Tested subset | IR-native supports source-order initialization from literals, earlier strings, concatenation, and calls returning `char[]`; arrays of strings are unsupported. |
| Array parameters and dimension metadata | Tested subset across ranks | Tested subset for supported rank/type combinations | AST supports broader rank and subarray forwarding. IR-native supports rank-one/two `int[]` and rank-one `char[]`, with explicit limitations. |
| Nested array literals / omitted slots | Tested subset | Tested subset for supported integer-array ranks | Short initializers and omitted entries zero-fill; unsupported nested shapes are rejected. |
| String initialization/copy/truncation | Tested subset | Tested subset | Fixed-capacity strings reserve NUL space and truncate safely. Runtime-sized string initialization is not implemented. |
| String concatenation `+` | Tested subset | Tested subset | Dynamically sized result can be printed, compared or copied into bounded storage; allocation cleanup is process-exit based. |
| String content comparisons | Tested subset | Tested subset | `=`, `!=`, `<`, `>`, `<=`, `>=` compare contents. |
| String append `+:` | Tested subset | Tested subset | Bounded append and self-append are covered; truncation preserves NUL termination. |
| String-returning functions | Tested subset | Tested subset | Owned-copy return convention is tested for literals, concatenation, parameters and local buffers. |
| Arrays of strings | Rejected/unsupported | Rejected/unsupported | Not implemented. |

## Optimization, debugging, and platforms

| Feature | Status | Evidence / remaining gap |
|---|---|---|
| AST constant folding | Tested subset | Safe literal-only folding exists; overflow/division hazards are deliberately not folded. Runtime arithmetic semantics still need a full audit. |
| IR constant propagation, dead-code removal, LICM, induction analysis, strength reduction | Tested subset for conservative patterns | `--release --dump-ir` and IR-native supported programs use selected passes; not a general optimizer and not all transforms have broad proof coverage. |
| Debug source-line mapping | Partial | Generated C uses `#line`; variable and expression mappings and consistent native debugger behavior are incomplete. |
| Native object generation | Tested on Linux x86-64 | Object generation and debug information have Linux evidence. |
| Linux build and smoke suite | Tested | Clean strict C++98 build and full smoke suite pass in the current environment. |
| Visual Studio 2022 / Windows | Unverified | Project files are present; native build/run is required. |
| Xcode 26+ / macOS | Unverified | Project/workspace files are present; native build/run is required. |

## Immediate conformance gaps discovered in this audit

1. The IR-native backend previously rejected `^`; it now emits `pow` and infers a `real` result. The new `tests/expression_precedence.shl` verifies precedence, associativity, conversions, global initialization, and output parity.
2. Both backends now guard scalar integer `+`, `-`, `*`, `/`, `%`, unary negation, and scalar integer compound `+:`/`-:` assignments. They check division/remainder by zero and the signed-minimum divided or remaindered by -1 case. Loop-update arithmetic, array-dimension arithmetic, and optimizer rewrites still require a dedicated audit.
3. Both backends now check `int(value)` finite/range constraints and `char(value)` finite 0..255 constraints. Tests cover finite out-of-range values, a non-finite value produced by power, and negative/too-large character conversions. Exact endpoints and every conversion path still require audit; real division-by-zero and non-finite result semantics remain open.
4. Multi-output functions, runtime-sized arrays, rank-three-or-higher arrays, most borrowed library functions, and general cross-file imports remain unsupported in the IR-native backend.
5. Windows and macOS native builds remain unverified.
