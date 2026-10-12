> Current Rev3 status: [TMS6747_REV3_QUALIFICATION.md](TMS6747_REV3_QUALIFICATION.md). All sections below are historical.

## Latest phase: IR-native global arrays

Extended the opt-in `--ir-native` backend to fixed-size, rank-one and rank-two global `int` arrays. Global storage is emitted as flat row-major arrays, starts zero-filled, and receives nested literal initializers in source order before `main` begins executing. Short initializers and omitted entries remain zero; nested rows are copied using the destination row stride. Functions can index and mutate global arrays, pass them to matching-rank array parameters with dimension metadata, and reassign whole arrays with clear-then-copy zero-fill behavior. `.row`, `.col`, `.dim(axis)`, and `len(array)` work for these global arrays. The backend validates positive literal dimensions, rejects unsupported global types/ranks, and checks initializer shape against the destination. Regression tests compare AST and IR-native output for source-order initialization, nested globals, attributes, forwarding, mutation, and reassignment, and compile generated IR C with strict C99 warnings-as-errors. The full Linux smoke suite and strict C++98 build pass.

Limitations: this opt-in backend still rejects rank-three-or-higher arrays, non-integer globals, subarray-view arguments, and unsupported IR operations. The default backend remains AST-to-C; Visual Studio 2022 and Xcode 26+ builds have not been independently verified.

## Latest phase: IR-native two-dimensional integer arrays

Extended the opt-in `--ir-native` backend to fixed-size, two-dimensional local `int` arrays. Nested indexing is lowered as one row-major access, with independent bounds checks for each dimension. Nested literal initialization and reassignment support short rows and omitted scalar entries: destination storage is cleared first, then each supplied row is copied using the destination row stride, preserving zero-fill semantics. The backend currently supports ranks one and two, literal positive dimensions, and explicit nested row literals; rank-three arrays, array parameters, and global arrays remain rejected. Regression tests compare AST and IR-native output for nested initialization, reads/writes, reassignment, and zero-fill, and check invalid row/column indices. Strict C++98 compilation and the full Linux smoke suite pass.

Limitations: this remains an experimental subset. Native executable generation uses IR only when `--ir-native` is selected; default generation remains AST-to-C. Visual Studio 2022 and Xcode 26+ builds remain independently unverified.

## Latest phase: IR-native fixed-size one-dimensional integer arrays

Extended the opt-in `--ir-native` backend to support fixed-size one-dimensional local `int` arrays. It lowers declarations to checked zero-filled heap allocations, handles flat array-literal initialization and reassignment (clearing the destination first so omitted elements become zero), and lowers indexed reads and writes through runtime bounds checks for negative and upper-bound indices. Array literal temporaries are represented as zero-initialized C arrays; generated code frees local allocations before explicit returns and at normal function exit. Regression tests compare AST-to-C and IR-native output for initialization, mutation, zero-filling and reassignment, and check negative and upper-bound failures. Multidimensional arrays, array parameters, global arrays, and non-`int` arrays are still rejected explicitly. Strict C++98 compilation and the full Linux smoke suite pass.

Limitations: this is an experimental IR-native subset; default native code generation still uses the AST-to-C backend unless `--ir-native` is selected. Visual Studio 2022 and Xcode 26+ builds remain independently unverified.

## Latest phase: IR-native scalar global support

Extended the opt-in `--ir-native` backend to support scalar `int` globals. Generated C declares globals with zero initialization and emits a guarded initialization routine that evaluates explicit initializers in source order. The routine is invoked at the start of `main`, before Shalimar statements execute; initializer expressions can call user-defined functions returning `int`. Functions can read and modify globals, and uninitialized globals begin at zero. The backend rejects global arrays and non-`int` globals explicitly. Regression tests compare AST-to-C and IR-native output for default-zero globals, source-order initialization, a global initializer calling a function, and reads/writes from functions; native object generation is also tested. Strict C++98 compilation and the full Linux smoke suite pass.

Limitations: the IR-native backend remains experimental and rejects arrays, borrowed library functions, real/char values, references, shadowed/repeated scalar declarations, and unsupported IR operations. It is not yet a replacement for the AST-to-C backend. Visual Studio and Xcode builds remain unverified.

## Previous phase: experimental IR-native scalar code generation

Added the opt-in `--ir-native` path for `--emit-c` and `--object`. It lowers optimized IR to C for scalar integer locals and parameters, user-function calls returning an integer or no value, scalar arithmetic/comparisons, branches, supported `while` and literal-step `for` loops, returns, and integer printing. Print-list separators and end-of-line markers are represented explicitly in the IR so the backend preserves multi-item output. Regression tests compare optimized strength-reduced loop output, scalar function-call output, and branch/loop output against the established backend and confirm unsupported array programs are rejected. The default AST-to-C backend remains unchanged unless `--ir-native` is specified.

## Previous phase: range-proven IR strength reduction

Release IR optimization rewrites a narrow, provably safe subset of multiplication-by-constant expressions into additive recurrences. It requires literal start/stride/limit and factor, a guaranteed nonzero-trip loop, a single unconditional body block, and proof that each product plus the recurrence's final post-loop update stays in the Shalimar `int` range. The preheader initializes a recurrence temporary; the step block increments it by `stride * factor`; the original multiplication is replaced by the recurrence value. Zero-trip, boundary-overflow, multi-block/conditional-body, or otherwise unsupported cases remain unchanged. Debug IR is preserved. The experimental `--ir-native` backend now consumes these rewrites for its supported scalar subset, while the default AST-to-C backend does not yet use them.

## Previous phase: IR constant propagation and dead-code elimination

Added a class-based `IROptimizer` for Release-mode IR. It performs conservative cross-block integer constant data-flow and safe folding, simplifies branches whose conditions become constant, removes unreachable blocks, and iteratively removes unused pure temporary computations. Potential overflow and division/remainder by zero are left unchanged. Debug-mode IR is not optimized. These passes feed the experimental `--ir-native` scalar backend when requested; the default backend remains AST-to-C.

# Shalimar Compiler implementation status

## Latest phase: IR-native string-literal output and precision control

The experimental `--ir-native` backend now supports string literals as direct print items, with escaped C string generation, and recognizes `prec(n)` as a print-format directive rather than a normal function call. Precision starts at seven decimal places, clamps values below -1 to the default sentinel and values above 24 to 24, and persists for subsequent real-valued output until changed. Precision directives do not print or add separators themselves, matching the AST backend's print-list behavior. Regression tests compare AST and IR-native output for string/real mixed lists and precision values 3, 0, and -1; generated C compiles with strict C99 warnings-as-errors, and IR-native object generation is covered. String support remains intentionally limited to direct print literals: string variables, parameters, return values, and string initializers are not supported. The normal AST-to-C backend remains the default. Visual Studio 2022 and Xcode 26+ builds remain independently unverified.

## Previous phase: initial Shalimar intermediate representation

Added a class-based AST-to-IR lowering layer and `--dump-ir` mode. The first IR format uses explicit three-address instructions, temporary values, basic blocks, branch/jump operations, successor edges, and source-line annotations. It lowers supported declarations, assignments, expressions, calls, printing, returns, `if`/`else`, `while`, and basic `for` loops. `--release --dump-ir` shows the IR after the existing conservative AST constant-folding pass. This is an initial inspection and architecture milestone: native object generation still uses the AST-to-C backend, and the IR is not yet the source of native code generation. SSA conversion, loop optimization, and complete IR semantics remain future work. Regression tests verify IR operations and control-flow edges; strict C++98 build and the full smoke suite pass in the current Linux environment.

This repository is a modular, C++98-source-compatible compiler bootstrap. It is not yet a conforming
implementation of the complete Shalimar 3.0 specification.

## Latest phase: source-level debug line mappings

Generated C now includes `#line` directives that map function bodies, global declarations and initializers, and parsed statements back to their original `.shl` file and source line. Because native object generation invokes the host C compiler with debug information enabled in debug mode, supported debugger line tables now identify the Shalimar source path and lines rather than only generated C locations. Regression tests compile a mapped source file, verify its output, and inspect the object debug line table when `readelf` is available. This is line mapping through generated C, not a dedicated Shalimar debug format; variable-name mapping, expression-level mapping, and consistent cross-toolchain debugger behavior remain incomplete.

## Latest phase: runtime-sized local arrays

Local array declarations may use runtime integer dimensions. The generated C evaluates each dimension once, rejects nonpositive extents, and allocates a zero-filled flat buffer on the heap with multiplication-overflow and allocation-failure checks. Runtime-sized one- and multidimensional arrays support indexing, `.row`, `.col`, `.dim(axis)`, `len()`, nested array-literal initialization, and array-literal reassignment. Generated cleanup frees each allocation at lexical scope exit and before explicit returns. Runtime-sized array literals are checked against actual dimensions at runtime. Regression tests cover dimensions, zero-fill, initialization, mutation, reassignment, and invalid dimensions/oversized initializers. Runtime-sized `char[]` string-literal initialization and runtime-sized globals remain unsupported; the compiler still implements only a subset of Shalimar 3.0.

## Previous phase: array-index bounds checking

Generated array indexing now routes every supported index through a checked helper. Negative indices and indices greater than or equal to the relevant dimension print a diagnostic to standard error and abort instead of performing an out-of-bounds read or write. Fixed-size local/global arrays derive extents from their C array types, while array parameters and runtime-sized local arrays use dimension metadata. Regression tests cover valid access, negative and upper-bound failures, multidimensional failures, and parameter indexing.

## Implemented and tested

- Class-based C++ frontend modules: lexer, parser, AST, semantic checker, code generator, and driver.
- Scalar `int`, `real`, and `char` declarations, assignments, arithmetic, comparisons, and conversions.
- Function definitions and calls with zero, one, or multiple scalar output types.
- Multi-output returns, such as `return (a / b, a % b)`, and multi-assignment, such as
  `<quotient, remainder> : quotientRemainder(17, 5)`.
- Both no-output main declarations: `fun main()` and `fun <> = main()`.
- Fixed-size and runtime-sized local arrays (including multidimensional arrays), nested multidimensional array initializers, element indexing and assignment. Runtime-sized arrays use checked zero-filled heap allocation and are freed on lexical scope exit or explicit return; runtime-sized string-literal initialization is not yet supported. Generated indexing checks reject negative indices and indices at or above the corresponding dimension. Assigning an array literal to a named local array resets all elements to zero, then writes supplied values.
- Array initializer rules for the implemented subset: nested literal shape is checked against rank, statically known dimensions reject excess entries, omitted slots and uninitialized elements are zero-filled, and short nested rows are zero-filled by aggregate initialization.
- One-dimensional `char[]` string initialization reserves the final array slot for the zero terminator; strings longer than the available text capacity are truncated.
- Local fixed-size array attributes: `.row`, `.col`, and `.dim(axis)`. Missing dimensions and negative/out-of-range axes return `-1`; scalar attribute access is rejected. `.dim(axis)` evaluates its axis expression once.
- Array parameters of any parsed rank are passed as flat C pointers with hidden runtime dimension metadata; runtime index checks use the passed dimensions. `.row`, `.col`, `.dim(axis)`, and `len()` use that metadata inside functions; multidimensional parameter indexing is lowered to row-major flat offsets, and subarrays can be forwarded to array parameters.
- Scalar reference parameters (`&name: type`) with copy-in/copy-back behavior, including array-element arguments. Reference arguments must be addressable and match the parameter type exactly. Each parameter is copied into a local value and copied back in parameter order when the function returns.
- `if`/`else if`/`else`, `while`, the two basic `for` forms, `break`, `continue`, and `return`.
- `?` and `??` output for supported scalar expressions, string literals, one-dimensional `char[]` text, and scalar `char` values.
- Semantic checks for variable names, scalar types, function arguments, output counts, return values,
  array indexing, and loop-control placement.
- Scalar and fixed-size global array declarations with source-order visibility and runtime initialization in source order. Global arrays support nested literals, zero-filled omitted entries, character-string initialization, indexing, mutation, and dimension attributes. Function definitions may appear before or after their calls; globals declared later are rejected with the declaration line.
- C source emission, syntax-check mode, and native object generation via the host C compiler.
- Debug object flags and release optimization flags forwarded to the native C compiler.
- Visual Studio 2022, Xcode, and CMake project definitions.
- Smoke tests for scalar functions, scalar globals and source-order diagnostics, arrays, checked valid and invalid indexing for local arrays, multidimensional arrays, and array parameters, local and parameter array attributes, rank-one and multidimensional array parameters, dimension metadata and subarray forwarding, multi-output functions, scalar reference copy-back (including aliased arguments), diagnostics, and native object generation.

## Known limitations

- Full Shalimar grammar, static semantics, and conversion insertion are not yet implemented.
- Array dimension metadata is passed through the generated C ABI for array parameters and supports fixed arrays passed into functions, including multidimensional indexing and subarray forwarding. Runtime-resizable arrays and full multidimensional array semantics remain incomplete. Bounds checks are implemented for supported array-index expressions.
- Array reference edge cases and full array/text printing are incomplete.
- Array literals are supported as local and global array declaration initializers, and as assignments to named local or global fixed-size arrays, including nested multidimensional literals and omitted slots. Assignment first clears the full destination storage, then writes the provided leaves, so omitted elements become zero. Dimensions must be compile-time integer constants for literal initialization and assignment in this stage. Array-literal reassignment to array parameters is implemented using runtime dimension metadata. The destination storage is cleared first, provided values are written in row-major order, and a literal that exceeds a runtime dimension aborts instead of writing out of bounds.
- Global array declarations and initializers are implemented for fixed-size compile-time dimensions. Global arrays support literal reassignment from functions, including nested multidimensional literals and zero-filling of omitted values.
- Full cross-file `uses` imports are not implemented. The current stage resolves the listed built-in math functions and `len` within one source file.
- String append (`+:`) and full string mutation/copy semantics are not implemented. String literal escape sequences are intentionally not interpreted, matching the language reference. Numeric arrays do not yet print as aligned grids.
- The parser and checker do not yet implement every diagnostic rule or collect all checker errors.
- Source-level line mappings are emitted using C `#line` directives for functions, global declarations and initializers, and statements. Debugger behavior is still dependent on the native compiler; variable and expression-level mapping is incomplete.
- Release builds run a conservative AST-level integer constant-folding pass for safe literal-only arithmetic, comparisons, and logical operations. Operations that could overflow or divide by zero are left unchanged. The standard native backend still uses AST-to-C. An experimental `--ir-native` backend can emit C from optimized IR for a limited scalar-integer subset; it rejects unsupported constructs instead of silently falling back. SSA and broad IR-driven native code generation remain outstanding.
- No cross-target object writer is included. Each platform must compile and link using its own native
  toolchain, ABI, runtime, and libraries.
- Linux C++98 build and smoke tests are verified in the current environment. Visual Studio 2022 and Xcode
  builds have not been independently run here.

Unsupported features should be treated as outstanding work, not assumed to be fully conformant. The
original language reference is retained in [`SHALIMAR_LANGUAGE.md`](SHALIMAR_LANGUAGE.md).


## Latest phase: scalar globals and source-order visibility

Global initializers execute in declaration order before `main`. They may call a function defined later, provided that function does not refer to globals that are declared after the function's own source line. The later borrowed-library milestone below supersedes the earlier implementation note: `uses` directives are now parsed and a documented set of built-in library functions is resolved.


## Borrowed library functions (current milestone)

Implemented `uses name[, name ...]` declarations and semantic resolution for the documented subset of C math functions plus `len`. Calls must be explicitly borrowed; arity and numeric/array argument categories are checked. Borrowed names cannot be reused for variables or parameters. Pointer-based and variadic functions such as `memset` and `printf` are rejected with a reason. User-defined functions take precedence over a borrowed built-in name. The native C toolchain supplies the actual math function implementations.

Limitations: `len` requires the array name or supported array expression forms handled by the current checker; the generated ABI now carries dimensions for array parameters. Library behavior follows the host C math library, so edge-case details and floating-point behavior still require cross-platform conformance tests.


## Latest phase: strings and precision printing

Implemented and smoke-tested `char name[n] : "text"` initialization for one-dimensional local character arrays, inline string literal output, text output from local `char[]` variables and rank-one `char[]` parameters, and character output for scalar `char` expressions. Real scalar output uses fixed decimal places: seven by default. A `prec(n)` print-list directive changes precision for subsequent values, clamps to `-1..24`, and uses the default seven places when the setting is negative. Non-finite values and magnitudes at or above `1e15` use compact significant-digit formatting. `prec()` must have exactly one `int` argument; it prints no value itself.

Remaining print/string conformance work includes the specified trailing space after every printed item, aligned numeric-array grids, grid precision of six places, full string append (`+:`) semantics, and exact cross-platform behavior for extreme floating-point values. The current implementation is intentionally documented as a partial implementation rather than complete string conformance.


## Latest phase: array initializer semantics

The parser now preserves omitted array-literal slots. The checker validates nested literal shape against declared rank, rejects statically oversized initializer dimensions, and requires integer dimensions. Fixed-size arrays use compile-time extents; runtime-sized local arrays use runtime extent checks when nested literals initialize or reassign them. Generated C uses zero values for explicit empty slots and aggregate initialization for omitted trailing elements and short nested rows. `char name[n] : "text"` reserves the final slot for the terminator and truncates overlong text to at most `n - 1` bytes. Regression tests cover nested zero filling, omitted slots, excess entries, invalid multidimensional shape, and char-array truncation. Runtime-sized local array initialization with nested array literals is implemented; runtime-sized `char[]` string-literal initialization and runtime-sized global arrays remain unsupported.


## Latest phase: array-literal reassignment

Array literals can now be assigned to an existing named local array with `:`. Generated code clears the whole local array with `memset` before assigning literal leaves, so omitted entries and rows are reset to zero rather than retaining old values. The checker validates literal rank, shape, element types, and statically known extents. Tests cover one-dimensional reset behavior, multidimensional reset behavior, and oversized assignment rejection. Assignment to array parameters is implemented in the parameter-reassignment milestone; global arrays are implemented in the later global-array milestones.

## Previous phase: array-parameter dimension metadata

Array parameters now carry hidden integer dimensions in generated C calls and prototypes. The frontend permits rank-greater-than-one parameters, lowers their indexing through row-major offsets, and passes subarray pointers and remaining dimensions when forwarding a row to another function. `.row`, `.col`, `.dim(axis)`, and borrowed `len(array)` now work for array parameters; nonexistent axes still return `-1`. The smoke test exercises rank-one and rank-two metadata, element access, and subarray forwarding. This remains a partial array implementation: runtime-sized global declarations and complete array-reference semantics are not implemented. General index bounds checks were added in a later milestone. Fixed-size global arrays are implemented in the later global-array milestone below.


## Latest phase: global array-literal reassignment

Array literals can now be assigned to existing named global fixed-size arrays using `:` from inside functions. Generated code clears the entire global destination with `memset` before writing the supplied literal leaves, so omitted elements and nested rows are reset to zero. The semantic checker validates rank, shape, element types, and known dimensions, and rejects oversized assignments. Regression tests cover one-dimensional and multidimensional global arrays plus oversized assignment rejection. Array-parameter reassignment is covered by the later parameter-reassignment milestone.

## Previous phase: fixed-size global arrays

Global arrays now support fixed-size compile-time integer dimensions, nested array-literal initializers, omitted-slot and trailing zero filling, and one-dimensional `char[]` string initialization with terminator reservation and truncation. Global arrays can be indexed and mutated from functions, and `.row`, `.col`, `.dim(axis)`, and borrowed `len(array)` work for them. Global initialization runs in source order alongside scalar globals. Regression tests cover one- and two-dimensional globals, string initialization, indexing, mutation, dimension attributes, `len`, and invalid non-integer dimensions. Global array-literal reassignment is covered by the later global-reassignment milestone. General array-index bounds checking was added in the later checked-indexing milestone.


## Latest phase: array-parameter literal reassignment

Array parameters can now be assigned nested array literals with `:`. Generated code clears the destination using the product of its runtime dimensions, writes provided values in row-major order, and leaves omitted values as zero. The checker validates literal rank and element types; because parameter extents are only known at runtime, generated guards abort if any literal dimension exceeds the actual argument's dimension. Tests cover one-dimensional and multidimensional reassignment and a runtime oversized-assignment failure. General indexing bounds checks were added in the later checked-indexing milestone.


## Latest phase: release constant folding

Added a class-based optimizer pass that walks global declarations, function bodies, statements, array dimensions, and expressions. In Release mode, safe literal-only integer arithmetic (`+`, `-`, `*`, `/`, `%`), comparisons, and logical operators are folded before C generation. The pass conservatively leaves potential integer overflow and divide-by-zero expressions unchanged, and is disabled in Debug mode to preserve generated source expressions for debugging. Regression tests compare Release and Debug generated C and execute the Release result. This is an AST-level optimization pass, not yet a dedicated intermediate representation or loop optimizer.


## Latest phase: cross-block IR data-flow

The Release IR optimizer now propagates known integer constants across basic-block edges using a conservative forward data-flow fixed point. At joins it keeps only values that agree across reachable predecessors, simplifies branches after substituting facts, removes blocks made unreachable, and repeats analysis so later passes can benefit from simplified control flow. Existing safety checks continue to avoid folding potential integer overflow and division/remainder by zero. Tests cover propagation into successor blocks and ensure Debug IR remains unoptimized. This pass affects `--dump-ir` only; native code generation still follows the AST-to-C path. SSA and loop-invariant code motion remain future work.


## Latest phase: canonical loop induction-variable analysis

Release-mode IR optimization now recognizes a conservative subset of canonical `for` loops and records induction metadata in `--dump-ir`: induction variable, literal start, nonzero literal step, literal limit, unique preheader, loop header, and step block. The analysis rejects loops with non-literal bounds/strides, ambiguous preheaders, body writes to the induction variable, or calls/multi-output calls that could mutate aliases. Debug IR remains unchanged. Regression tests check successful recognition, rejection of a modified induction variable, Debug preservation, and unchanged execution output. This is analysis metadata only; induction-variable strength reduction has not yet been implemented, and the IR remains separate from native code generation.


## Latest phase: experimental IR-native scalar code generation

Added the opt-in `--ir-native` path for `--emit-c` and `--object`. It lowers the optimized IR to C for a deliberately limited subset: scalar integer locals and parameters, user-function calls returning an integer or no value, scalar arithmetic/comparisons, branches, supported `while` and literal-step `for` loops, returns, and integer printing. The backend rejects global declarations, arrays, borrowed library functions, real/char values, reference parameters, unsupported operations, and shadowed/repeated scalar declarations. Print-list separators and end-of-line markers are represented explicitly in the IR so the backend preserves multi-item output. The default AST-to-C backend remains unchanged and is used unless `--ir-native` is specified. Tests compare the IR-native strength-reduced loop result and scalar call/print output, and build an IR-native native object. Linux strict C++98 build and full smoke suite pass.

Limitations: this is an experimental subset, not a general replacement for AST-to-C; it does not support globals, arrays, strings, borrowed library calls, or real arithmetic. Visual Studio and Xcode builds remain unverified.


## Latest phase: IR-native array parameters and dimension metadata

The experimental IR-native C backend now accepts rank-one and rank-two integer array parameters. Function prototypes and definitions carry hidden dimension integers for each array parameter; call sites pass the extents for local arrays or forward the received metadata when passing an array parameter onward. Parameter indexing uses row-major offsets and runtime bounds checks. Array parameters support `.row`, `.col`, `.dim(axis)`, and borrowed `len(array)`. Whole named arrays can be forwarded to matching-rank parameters, and indexed mutation plus array-literal reassignment work through the pointer parameter. Reassignment clears the actual destination and emits runtime guards when a literal exceeds the passed dimensions. Regression tests compare AST and IR-native output for forwarding, mutation, reassignment, metadata, and both one- and two-dimensional parameters; failure tests cover out-of-bounds access and oversized parameter reassignment. Subarray views such as forwarding `matrix[1]` as a rank-one array are not supported by this IR backend yet. Rank-three-or-higher arrays, non-integer array parameters, subarray-view arguments, and general external ABI interoperability remain unsupported.


## Latest phase: IR-native scalar real and char support

The experimental IR-native backend now supports scalar `real` and `char` globals, locals, parameters, and single-value function returns in addition to the existing integer subset. C code generation infers real temporary types for supported floating-point expressions and real-returning calls; comparisons and logical results remain integer-valued. Character variables use an integer representation internally and are emitted with character output semantics. Real output matches the existing backend's default formatting for tested finite values. Regression tests compare AST and IR-native output for source-order real global initialization, real arithmetic, typed function arguments/returns, character mutation via a function, and output; the IR-generated C compiles with strict C99 warnings treated as errors.

Limitations: `real` and `char` arrays, reference parameters, string values, and `prec()` control in IR-native code remain unsupported. The default AST-to-C backend is unchanged. Visual Studio 2022 and Xcode 26+ builds remain independently unverified.


## Latest phase: IR-native char-array strings and parameters

The experimental IR-native backend now supports local one-dimensional `char[]` string buffers, string-literal initialization, bounded whole-string assignment/copy, direct string output, character indexing, and `len(string)` as declared capacity. Generated C uses a capacity-aware copy helper that truncates to at most capacity minus one and writes a NUL terminator. Rank-one `char` array parameters carry hidden capacity metadata and support printing, reassignment, indexing, capacity queries, and forwarding to compatible functions. The AST-to-C backend was also corrected to emit bounded copy operations for assignments to existing fixed-size char arrays instead of invalid C array assignment. Regression tests compare AST and IR-native results for initialization, truncation, assignment, parameter forwarding, mutation through parameters, and capacity behavior. Strict C++98 build, strict C99 compilation of IR-native output, and the full smoke suite pass.

At this phase string concatenation/comparison were not yet implemented; the subsequent phase below adds content comparisons and bounded `+:` append. Limitations that remain are dynamic string concatenation expressions using `+`, string-returning functions, global char arrays in the IR-native backend, and arrays of strings. The IR-native backend is still experimental and opt-in; Visual Studio 2022 and Xcode 26+ builds remain independently unverified.


## Latest phase: IR-native string comparisons and bounded append

String equality and ordering now compare string contents via `strcmp` in the AST-to-C and experimental IR-native backends. Supported operators are `=`, `!=`, `<`, `>`, `<=`, and `>=`, with two string operands. `+:` appends to fixed-capacity `char[]` buffers, truncates to available capacity, and preserves the terminating NUL byte. The append helper safely handles self-append by copying the source before mutation. Regression tests compare both backends for content comparisons, literal and variable operands, bounded append, truncation, and self-append. Strict C++98 build, strict C99 compilation of IR-native generated C, and the full smoke suite pass.

Limitations: dynamic string concatenation expressions using `+`, string-returning functions, global `char[]` storage in the IR-native backend, and arrays of strings remain unsupported. The IR-native backend remains experimental and opt-in; Visual Studio 2022 and Xcode 26+ builds remain independently unverified.


## Latest phase: dynamic string concatenation

String concatenation with `+` is now supported in the AST-to-C and experimental IR-native backends when both operands are strings. Generated results are dynamically sized and can be printed, compared by content, or assigned into an existing fixed-capacity `char[]` buffer with safe truncation and NUL termination. Generated C tracks these temporary allocations and releases them at process exit. Regression tests compare AST and IR-native output for literal and variable concatenation, comparison of concatenated values, bounded assignment, and truncation; the IR-native generated C passes strict C99 warnings-as-errors.

Limitations: string-returning functions, global `char[]` storage in the IR-native backend, and arrays of strings remain unsupported. The IR-native backend is experimental and opt-in. Visual Studio 2022 and Xcode 26+ builds remain independently unverified.


## Latest phase: IR module separation and AST string returns

The former monolithic `src/IR.cpp` (over 140 KB) has been split into three role-based translation units: `IRBuilder.cpp` handles AST-to-IR lowering and IR dumps; `IROptimizer.cpp` owns constant propagation, loop analysis, strength reduction, and other IR passes; `IRCodeGenerator.cpp` owns supported-subset validation and native C emission. CMake, Visual Studio, and Xcode project files reference the new modules. The strict C++98 build and the complete smoke suite pass after the split.

The function output-list parser now accepts `char[]` as an output type, e.g. `fun <char[]> = makeText()`. The default AST-to-C backend returns an owned string copy so values remain valid after local-array cleanup. Tests cover returning literals, returning concatenation expressions, returning a string parameter expression, bounded copy into caller-owned fixed-capacity buffers, and content comparison. The experimental IR-native backend still rejects string-returning functions with a clear diagnostic; global `char[]` storage and IR-native string-return ABI support remain future work. Visual Studio 2022 and Xcode 26+ builds are still not independently verified in this environment.


## Latest phase: IR-native string-return ABI

The experimental IR-native backend now accepts `char[]` function return types alongside scalar `int`, `real`, and `char` returns. Function declarations and definitions use `char *` in generated C, and calls produce string-valued IR temporaries. On each string return, generated code duplicates the expression before releasing local array allocations; this supports returning string literals, concatenation expressions, string parameters, and local fixed-capacity `char[]` arrays without returning pointers into freed storage. The regression fixture `tests/string_return.shl` now runs through both AST and IR-native backends and compares the expected output. Strict C++98 compilation, strict C99 warnings-as-errors for the IR-generated C, and the full smoke suite pass.

Remaining limitations: global `char[]` storage and arrays of strings in the IR-native backend. Visual Studio 2022 and Xcode 26+ builds remain independently unverified.


## Latest phase: IR-native global string buffers

The experimental IR-native backend now supports fixed-size rank-one global `char[]` buffers. These globals are emitted as static C character arrays, initialized in source order from string literals with capacity-bounded copying and NUL termination (including initializer truncation), and are available across functions. Reads, indexed mutation, whole-string reassignment, bounded `+:` append, concatenation into the global destination, and `len` capacity queries are covered by regression tests. The new `tests/ir_native_global_strings.shl` fixture is compiled as strict C99 with warnings treated as errors and also produces a native object. Strict C++98 build and the full smoke suite pass.

The next phase adds non-literal global string initializers that reference earlier globals, concatenate strings, copy existing strings, or call `char[]`-returning user functions. Arrays of strings remain unsupported. The AST-to-C backend's whole-array assignment support for global strings remains a separate limitation. Visual Studio 2022 and Xcode 26+ builds remain independently unverified.


## Latest phase: IR-native global string expression initializers

Global rank-one `char[]` buffers can now be initialized from earlier global string values, concatenation expressions, copies of existing strings, and calls to user-defined functions returning `char[]`. These expressions are evaluated in source order in the generated global-initialization routine, and the result is copied into the declared capacity with truncation and NUL termination. The new/expanded fixture `tests/ir_native_global_string_initializers.shl` checks global references, concatenation, copy, truncation, and a string-returning initializer function. The full smoke suite passes, including strict C99 warnings-as-errors for the generated IR-native C and object generation.

Remaining limitations: arrays of strings and global initializers requiring unsupported expressions/signatures. The AST backend's global whole-array assignment limitation remains separate. Visual Studio 2022 and Xcode 26+ builds are not independently verified.


## Latest phase: IR-native scalar reference parameters

The experimental IR-native backend now supports scalar `&` parameters for `int`, `real`, and `char`. Generated C passes a pointer, copies the value into a local parameter variable, and copies it back before explicit returns and at normal fallthrough. The Release optimizer clears constant facts before rewriting call operands because a reference call may mutate the actual argument; this prevents an addressable variable from being replaced with a constant in the call. Regression coverage compares AST and IR-native output for local and global variables, all three scalar types, explicit-return copyback, fallthrough copyback, and native object generation. Reference arguments can also address elements of rank-one and rank-two `int` arrays; generated C computes the element address with the same runtime bounds checks as ordinary indexing. Computed reference expressions and `char[]` element references remain unsupported and are rejected with a diagnostic. Strict C++98, strict C99 generated-code compilation, and the full smoke suite pass. Visual Studio 2022 and Xcode 26+ builds remain independently unverified.


## Latest phase: IR-native indexed scalar references

Calls to scalar `&` parameters can now pass indexed elements of rank-one or rank-two `int` arrays, such as `bump(values[1])` or `bump(matrix[row][column])`. The IR builder preserves the lvalue base and index expressions instead of lowering the argument as a value read; the C backend forms an address and applies runtime bounds checks before the call. Tests compare AST and IR-native results for rank-one and rank-two element mutation, and generated C passes strict C99 warnings-as-errors. Computed expressions and `char[]` elements passed by reference remain unsupported. The full smoke suite passes.


## Latest phase: IR-native char-string element references

Scalar `&value: char` parameters can now receive an element of a rank-one `char[]` string. The IR builder preserves the indexed lvalue, and the C generator evaluates and bounds-checks the index once, copies the character into a correctly sized scalar proxy, calls the function, and copies the updated character back to the same element. This preserves the target element even if another by-reference argument changes the index variable during the call. The regression fixture `tests/ir_native_char_reference_element.shl` checks this case, while the existing rank-one and rank-two integer-array reference tests remain covered. Generated C passes strict C99 warnings-as-errors and the full smoke suite passes. Computed expressions passed by reference remain unsupported.

## Latest phase: C++98 AST ownership guardrails

Added a private-copy `NonCopyable` base for AST expression nodes, statement nodes, `FunctionDef`, and `Program`. These types own pointer-linked children, so accidental shallow copying could otherwise lead to double deletion. Value-like records such as `Parameter` and `LibraryUse` remain copyable. Also hardened AST constant-fold replacement to allocate the replacement node before deleting the old node, preserving the original expression if allocation throws. Added `docs/MEMORY_SAFETY_AUDIT.md` and `docs/IMPLEMENTATION_AUDIT.md` to record the ownership risks, completion gates, and remaining work. The strict C++98 build and full smoke suite pass; parser exception-safe construction and a comprehensive sanitizer review remain future work.


## Latest phase: expression parity audit and IR-native power/conversions

Added `docs/FEATURE_PARITY.md`, a source-derived matrix comparing the default AST-to-C backend with the experimental IR-native backend, and `docs/SPEC_FREEZE.md` / `docs/SPEC_QUESTIONS.md` to distinguish the language-level reference from implementation status and record unresolved conformance decisions. The IR-native backend now supports `^` through `pow` with a `real` result, and handles `int(x)`, `real(x)`, and `char(x)` as conversion casts rather than unresolved user-function calls, including global initializers. The AST print type analysis now recognizes direct `char(x)` conversions as character output. `tests/expression_precedence.shl` compares both backends for right-associative power, unary-minus precedence, arithmetic precedence, logical precedence, conversions, function calls, and global expression initialization. The clean strict C++98 build and full smoke suite pass on Linux.

Numeric conformance remains incomplete: signed integer overflow, division/remainder by zero, and out-of-range `int`/`char` conversions are not yet guarded comprehensively. These are explicitly tracked in `docs/SPEC_QUESTIONS.md`; no claim of complete Shalimar 3.0 conformance is made. Windows/MSVC and macOS/Xcode builds remain unverified.


## Latest phase: checked scalar integer arithmetic and numeric conversions

Both the AST-to-C and experimental IR-native code generators now emit runtime guards for scalar integer addition, subtraction, multiplication, division, remainder, unary negation, and scalar integer compound `+:`/`-:` assignments. Addition, subtraction, and multiplication use an `int64_t` intermediate and reject results outside the signed 32-bit range. Division and remainder reject zero divisors and the `INT32_MIN` with `-1` overflow case. `int(value)` checks that the source is finite and its truncated value is representable as a signed 32-bit integer; `char(value)` checks finiteness and the 0..255 range before conversion. The regression suite tests valid AST/IR output parity and deterministic diagnostics for overflow, division/remainder by zero, invalid int conversions, non-finite conversion input, and invalid char conversions in both backends. Strict C++98 compilation and the full Linux smoke suite pass.

This is not yet a complete proof of numeric safety: loop induction updates, all array-dimension arithmetic, optimizer transformations, and real non-finite/division-by-zero semantics still require a dedicated audit. Generated AST C still emits some unused static helper warnings in fixtures that do not use those helpers. Visual Studio 2022 and Xcode 26+ native builds remain unverified.
