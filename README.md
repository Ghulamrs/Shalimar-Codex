# Shalimar TMS6747 Windows Merge Rev3

Current checkpoint: [qualification, tool paths and reproduction](docs/TMS6747_REV3_QUALIFICATION.md). The own-tool pipeline passes 48/48 VM/SIM/CCS runtime cases. TI source debugging is separately verified. Full language coverage and own-tool DWARF remain incomplete. Start with [HANDOFF.md](HANDOFF.md).

The sections below preserve historical development notes.

# Shalimar rev6

Current source and tested Linux compilers. Read [the verified results and limitations](docs/REV6_TEST_RESULTS.md) and [revision history](docs/REVISION_HISTORY.md) before interpreting older status sections below.

The 12 original supplied programs pass all four Linux Debug/Release combinations through AST-to-C. Compiler AST ownership uses container pools and index handles, with zero explicit compiler allocation operations. The experimental IR-native backend and complete normative-language coverage remain limited. The generated C runtime retains its own allocation helpers.

Linux quick start:

```sh
./bin/linux-x86_64/Release/shalimarc tests/original/gcd.shm --object gcd.o --release
cc gcd.o -lm -o gcd
./gcd
sh tests/run-debug-release.sh
```

Use bin/linux-x86_64/Debug/shalimarc for the Debug compiler build. Visual Studio 2022 and Xcode project files are included; native Windows/macOS verification has not been performed here.

---

# Shalimar Compiler

Shalimar is a modular, class-based compiler bootstrap implemented in C++98-compatible source. It currently parses and checks a documented subset of the Shalimar language, emits C99 for native toolchain compilation, and provides an inspectable three-address intermediate representation (IR).

**This is a partial implementation, not yet a conforming implementation of the complete Shalimar 3.0 specification.** Linux x86-64 is the currently verified target. Visual Studio 2022 and Xcode 26+ project files are included, but those native builds have not been independently verified in this environment.

## Build

```sh
cmake -S . -B build -DCMAKE_CXX_FLAGS='-Wall -Wextra -pedantic -Werror'
cmake --build build
```

The compiler source requires C++98. The generated C backend uses C99 and a platform-native C compiler.

## Run

```sh
build/shalimarc examples/hello.shl --check
build/shalimarc examples/hello.shl --emit-c hello.c --release
build/shalimarc examples/hello.shl --emit-c scalar.c --release --ir-native
build/shalimarc examples/hello.shl --object hello.o --debug
build/shalimarc examples/hello.shl --dump-ir
build/shalimarc examples/hello.shl --dump-ir --release
```

`--release` enables the AST-level safe constant-folding pass. For `--dump-ir`, it additionally enables conservative cross-block constant data-flow, safe folding, constant-branch simplification, unreachable-block removal, dead pure-temporary elimination, conservative loop-invariant code motion for total comparison/logical operations, canonical loop induction-variable analysis, and conservative, range-proven IR strength reduction for eligible induction-variable multiplications. The data-flow pass intersects facts at control-flow joins and recomputes after branch simplification. Induction analysis reports only loops with literal start, stride, and limit, a unique preheader, and no body writes to the induction variable or calls that could mutate aliases. Strength reduction rewrites only a narrow subset: a single unconditional body block, literal nonzero-trip loop bounds and stride, literal factor, and proven int-range safety for every product and the final recurrence update. Unsupported or unsafe loops are left unchanged. `--debug --dump-ir` leaves the IR unoptimized.

**IR-native backend (experimental):** `--ir-native` emits C from optimized IR for a deliberately limited subset: scalar `int`, `real`, and `char` locals, parameters, and globals; direct string-literal output; `prec(n)` print precision control; local one-dimensional `char[]` strings with literal initialization, bounded assignment/copy, truncation with a reserved NUL terminator, output, indexing, `len`, and rank-one `char` array parameters with hidden capacity metadata; fixed-size rank-one and rank-two global `int` arrays plus rank-one global `char[]` strings with source-order initialization; source-order scalar and array initialization (including calls to supported user-defined scalar functions for initializers); fixed-size one- and two-dimensional local `int` arrays using flat row-major storage; rank-one and rank-two `int` array parameters with hidden runtime dimension metadata; whole-array forwarding to matching-rank parameters, including global arrays; nested integer array-literal initialization and reassignment with zero-fill semantics; dimension-aware checked array reads and writes; `.row`, `.col`, `.dim(axis)`, and borrowed `len(array)` for supported arrays; typed scalar user functions; basic branches and loops; and type-aware scalar printing. Global integer arrays are zero-initialized, initialized in source order, and can be passed to array-parameter functions, mutated, and reassigned. Global `char[]` buffers are statically allocated, initialized from string literals with safe truncation, and can be read, indexed, reassigned, appended to, and accessed across function calls. Tests cover initialization truncation and global mutation from multiple functions. Integer and character-array parameters can be forwarded to matching-rank parameters; runtime guards validate index bounds and integer-array literal dimensions against actual extents. String assignments safely truncate to destination capacity and preserve a NUL terminator. String concatenation with `+` allocates a dynamically sized result that can be printed, compared by content, or copied into a bounded `char[]`; concatenation allocations are reclaimed at process exit. String-returning functions with `char[]` output are supported by both the default AST-to-C and experimental IR-native backends. Each backend returns an owned copy so returned literals, concatenations, parameters, and local buffers remain valid after function-local storage is released. Global `char[]` storage is supported by the IR-native backend with source-order initialization from literals, earlier global strings, concatenation expressions, and calls to `char[]`-returning functions; arrays of strings remain unsupported. It rejects borrowed library functions other than `len`, references, rank-three-or-higher arrays, subarray-view arguments, incomplete nested row literals, non-literal global-array dimensions, and unsupported control-flow or expression forms with a diagnostic rather than silently falling back. Without `--ir-native`, the established AST-to-C backend remains the default. `--object ... --release --ir-native` compiles the IR-generated C into a native object. This remains an experimental subset, not a complete replacement for the AST backend.

## Project layout

- `include/`, `src/` — C++98 frontend, semantic checker, AST optimizer, and role-separated IR modules (`IRBuilder.cpp`, `IROptimizer.cpp`, `IRCodeGenerator.cpp`).
- `tests/` — Shalimar regression fixtures and `run-smoke.sh`.
- `docs/STATUS.md` — implemented features, limitations, and phase history.
- `docs/IMPLEMENTATION_AUDIT.md` — verified baseline and completion gates.
- `docs/FEATURE_PARITY.md` — AST-to-C versus IR-native support matrix and conformance gaps.
- `docs/SPEC_FREEZE.md` / `docs/SPEC_QUESTIONS.md` — specification authority, change policy, and unresolved semantic questions.
- `docs/MEMORY_SAFETY_AUDIT.md` — C++98 ownership findings and remediation plan.
- `ShalimarCompiler.sln`, `ShalimarCompiler.vcxproj` — Visual Studio project files.
- `ShalimarCompiler.xcodeproj/`, `ShalimarCompiler.xcworkspace/` — Xcode project and workspace.
- `bin/linux-x86_64/shalimarc` — prebuilt Linux x86-64 compiler executable.

## Tests

```sh
sh tests/run-smoke.sh
```

The smoke suite builds the compiler, exercises implemented language features (including expression precedence/power/conversions), checks diagnostics and debug mappings, and verifies release/debug optimization behavior.

## Latest phase: IR-native scalar reference parameters

The experimental IR-native backend now supports scalar `&` parameters of type `int`, `real`, or `char`. Generated C passes pointers, copies the incoming value into a local variable, and copies it back on explicit returns and normal fallthrough. Release constant propagation now clears facts before rewriting call operands so addressable arguments are not incorrectly replaced with constants. Regression coverage compares AST and IR-native output for local and global variables, all three scalar types, explicit return, and fallthrough. Scalar reference arguments in the IR-native backend accept named scalar variables and elements of rank-one or rank-two `int` arrays, with runtime bounds checks on indexed addresses. Computed expressions remain unsupported. Elements of rank-one `char[]` strings are supported through a checked-indexed copy-in/copy-back proxy, preserving the string element width and NUL terminator when the resulting value is in range. Strict C++98, strict generated C99, the smoke suite, and native object generation pass.

## Current IR optimization milestone

The class-based `IROptimizer` performs conservative cross-block integer constant data-flow, safe arithmetic/comparison/logical folding, constant-branch simplification, unreachable-block removal, iterative removal of unused pure temporary computations, loop-invariant code motion for comparisons and logical operations with stable operands, canonical induction-variable analysis for supported `for` loops, and range-proven strength reduction for multiplication of an induction variable by a literal constant in a single unconditional body block. The optimizer rewrites the Release IR multiplication to a recurrence temporary initialized in the preheader and incremented in the loop step block, but only when literal bounds and stride prove a nonzero trip count and all products, including the final recurrence update, remain in the Shalimar int range. Unsupported or unsafe cases remain unchanged. Loop motion excludes arithmetic operations because signed overflow in generated C may be undefined, and it declines to move named operands across loops containing calls or multi-output calls. Induction analysis accepts only literal start/stride/limit values and rejects loops that modify the induction variable in the body or contain calls. Potential overflow and division/remainder by zero are not folded. The IR passes are used by `--dump-ir --release` and by the experimental `--ir-native --release` backend for its supported scalar and fixed-size one- and two-dimensional integer-array subset, including array parameters with runtime dimension metadata and global integer arrays. The standard backend remains AST-to-C; SSA and broader IR-driven native code generation remain future work.

See [the implementation status](docs/STATUS.md) and the original [Shalimar language reference](docs/SHALIMAR_LANGUAGE.md) for more detail.

## Latest phase: IR-native string-literal output and precision control

The opt-in `--ir-native` backend now emits direct string-literal print items with C-safe escaping and supports `prec(n)` directives in print lists. Precision defaults to seven decimal places, values below -1 are clamped to the default sentinel, values above 24 are clamped to 24, and a negative precision sentinel restores the default formatting. Precision directives do not themselves print a value or introduce a separator; later real values use the current precision, matching the AST backend. Regression tests compare AST and IR-native output for string literals, mixed string/real print lists, and precision values of 3, 0, and -1. The generated IR-native C compiles with strict C99 warnings-as-errors, and native object generation is tested.

At this phase strings were supported only as print literals; later phases add local `char[]` variables, parameters, bounded assignment, comparisons, and append. String-returning functions, global `char[]` storage in the IR-native backend, and arrays of strings remain unsupported. Real arrays and reference parameters remain unsupported. The IR-native backend is opt-in and experimental; Visual Studio and Xcode builds still need independent verification.


## Latest phase: IR-native char-array strings and parameters

The experimental `--ir-native` backend now supports local one-dimensional `char[]` string buffers, string-literal initialization, bounded whole-string assignment/copy, direct string output, indexing as `char`, and `len(string)` as the declared capacity. Generated C uses capacity-aware copying that truncates to at most capacity minus one and always writes a terminating NUL. Rank-one `char` array parameters carry hidden capacity metadata; functions can print, reassign, index, inspect the capacity of, and forward a string parameter to another compatible function. The existing AST-to-C backend was also corrected to copy string values into existing fixed-size `char[]` destinations rather than emitting invalid C array assignment. Regression tests compare AST and IR-native output for string initialization, truncation, assignment, parameter forwarding, mutation through a parameter, and capacity behavior. Strict C++98 compilation, strict C99 compilation of IR-generated C, and the full smoke suite pass.

Limitations: this phase does not add string concatenation/comparison, string-returning functions, global `char[]` storage in the IR-native backend, or arrays of strings. The IR-native backend remains opt-in; Visual Studio 2022 and Xcode 26+ builds remain unverified.


## Latest phase: IR-native string comparisons and bounded append

String equality and ordering comparisons now use string contents (via `strcmp`) in both the AST-to-C and experimental IR-native backends. The supported comparisons are `=`, `!=`, `<`, `>`, `<=`, and `>=`, with two string operands. The `+:` operator now appends text to a fixed-capacity `char[]` buffer, truncating to the available space while preserving a NUL terminator. Append is safe when the source and destination are the same buffer. Regression tests compare AST and IR-native behavior for all six comparisons, literal and variable operands, bounded append, truncation, and self-append; IR-generated C compiles with strict C99 warnings-as-errors.

Limitations: dynamic string concatenation expressions using `+`, global `char[]` storage in the IR-native backend and arrays of strings remain unsupported. The IR-native backend remains experimental and opt-in; Visual Studio 2022 and Xcode 26+ builds remain unverified.


## Latest phase: dynamic string concatenation

String `+` now concatenates two string operands in both the AST-to-C and experimental IR-native backends. The result is dynamically sized, can be printed or compared by content, and can be copied into an existing fixed-capacity `char[]` buffer (where assignment truncates safely). The generated C tracks concatenation allocations and frees them at process exit. Regression tests compare AST and IR-native behavior for literal concatenation, variable concatenation, content comparison, bounded assignment, and truncation; IR-generated C is compiled with strict C99 warnings-as-errors.

Limitations: global `char[]` storage in the IR-native backend and arrays of strings remain unsupported. The IR-native backend remains experimental and opt-in; Visual Studio 2022 and Xcode 26+ builds remain independently unverified.


## Latest phase: IR module separation and AST string returns

The former 148 KB `src/IR.cpp` was split by role into `IRBuilder.cpp` (AST-to-IR lowering and IR dumps), `IROptimizer.cpp` (data-flow, loop analysis, and optimization passes), and `IRCodeGenerator.cpp` (IR validation and C emission). CMake, Visual Studio, and Xcode project definitions now compile the three files directly.

The parser now accepts `fun <char[]> = function(...)` for a function that returns text. The AST-to-C backend returns an owned copy of the string, so returning a literal, a concatenation, or a string parameter does not return a pointer into a local buffer that is freed on function exit. Regression coverage checks returned literals, returned concatenations, copying into fixed-capacity strings, and content comparison. The experimental IR-native backend rejects string-returning functions with a clear diagnostic; support for the same string-return ABI in that backend remains future work. Strict C++98 build and the full smoke suite pass.


## Latest phase: IR-native string-return ABI

The experimental IR-native backend now supports `fun <char[]> = function(...)` string-returning functions. Generated C uses `char *` return types and duplicates the return expression before freeing function-local arrays, so returning literals, concatenation results, string parameters, or local fixed-capacity `char[]` buffers is safe. Calls can consume returned strings for bounded assignment, output, concatenation, and content comparison. Regression tests run the same string-return fixture through both AST and IR-native backends and compare output; the IR-generated C compiles with strict C99 warnings-as-errors. The C++98 build and full smoke suite pass.

Remaining IR-native string limitations include arrays of strings and global string initializers that depend on unsupported expressions or unsupported function signatures. The backend remains experimental; Visual Studio 2022 and Xcode 26+ builds remain independently unverified.


## Latest phase: IR-native global string expression initializers

Global rank-one `char[]` initializers can now reference earlier global string buffers, concatenate strings, copy another global string, and call a user-defined function returning `char[]`. Initialization remains in source order, so a function used by a later initializer can safely read globals initialized earlier. Generated C copies each resulting value into the fixed-capacity destination with truncation and NUL termination. Regression coverage is in `tests/ir_native_global_string_initializers.shl`; the smoke suite compiles its generated C as strict C99 with warnings treated as errors and checks a native object build.

Arrays of strings and unsupported global-initializer expressions remain outside the IR-native subset. The AST backend's separate global whole-array assignment limitation is unchanged. Visual Studio 2022 and Xcode 26+ still require independent native builds.

## Latest phase: IR-native char-string element references

A scalar `&value: char` parameter can now receive an element of a rank-one `char[]` string. The generated code evaluates and bounds-checks the index once, copies the character into a correctly sized scalar proxy, invokes the function, and copies the modified character back to the same element. This remains correct even if another by-reference argument changes the index variable during the call. The fixture `tests/ir_native_char_reference_element.shl` covers this case; rank-one and rank-two integer-array element references remain covered by their own fixtures. Computed expressions passed by reference remain unsupported.


## Current completion and memory-safety audit

The current Linux smoke suite passes, but the compiler remains a partial Shalimar implementation and Windows/macOS native builds are not independently verified. The repository now includes an implementation completion checklist and a C++98 memory-safety audit. The audit records AST ownership through raw pointers, parser error-path cleanup risks, the value-based IR model, and the separate lifetime rules required by generated C. It also records the optimizer exception-safety correction. See `docs/IMPLEMENTATION_AUDIT.md` and `docs/MEMORY_SAFETY_AUDIT.md` before planning further feature work.


## Latest phase: checked integer arithmetic and conversions

Both code-generation backends now emit runtime checks for scalar integer `+`, `-`, `*`, `/`, `%`, unary negation, and scalar integer compound `+:`/`-:` assignments. Integer overflow, division/remainder by zero, and the signed minimum divided or remaindered by `-1` terminate with a Shalimar diagnostic rather than relying on C undefined behavior. `int(value)` checks finite, signed-32-bit representability after truncation; `char(value)` checks finite values in the 0..255 range. `tests/run-smoke.sh` tests valid output parity and negative cases in both backends. Numeric safety is not yet exhaustive: loop-update arithmetic, every array-dimension path, optimizer rewrites, and real non-finite behavior still need auditing.

### No-output function shorthand (rev13)

`fun main() { ... }` and `fun greet() { ... }` are equivalent to `fun <> = main() { ... }` and `fun <> = greet() { ... }`. This shorthand is only valid for functions with zero declared outputs; value-returning functions retain their explicit output list. Both forms remain accepted for backwards compatibility.

## rev17 specification example regression

The original 12 `.shm` examples are retained in `tests/spec-examples/`.
Run `SHALIMAR_COMPILER=build/shalimarc sh tests/run-spec-examples.sh` to check each example and compile generated C objects in both Debug and Release modes. See `docs/REV17_VERIFICATION.md` for the verification scope and remaining limitations.
