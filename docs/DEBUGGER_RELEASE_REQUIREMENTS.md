# Rev3 debugger release requirements

Status: required for this revision; not yet satisfied. This requirement follows the user's decision that Shalimar must offer practical source and variable debugging comparable to C/C++ for the intended four-machine solution.

## Acceptance contract

For every implemented language construct on every supported native target, a debugger must be able to:

- Set a source breakpoint, resolve it to executable instructions, and stop on the intended statement.
- Step into, over and out of functions without losing the source position or caller stack.
- Inspect parameters, globals and initialized locals with their Shalimar names and correct types and values.
- Inspect array dimensions, elements and storage capacity. Character buffer capacity and current text length are distinct; displaying a string must not assume that every buffer is terminated.
- Respect lexical scope and shadowing. A name must not resolve to another variable's storage.
- Report unavailable or optimized-out values honestly. Uninitialized values must not be presented as initialized source values.
- Inspect nested calls and their stack frames, including a stop inside a runtime bounds or arithmetic failure.
- Associate diagnostics with the original source file and location.

Debug mode must retain useful source locations and variable lifetimes. Release mode must retain valid stack unwinding and, when debug symbols are requested, accurate locations for surviving values. Optimization may remove a value; it must not fabricate a value or silently attach its name to reused storage.

This is debugging parity for Shalimar constructs, not a requirement to add C++ language constructs such as templates, inheritance or exceptions.

## Shared design

Carry source file, line, column, lexical scope, stable variable identity, declared type, dimensions and parameter identity through AST and IR. Track each variable's live instruction ranges and concrete storage locations after instruction selection. Describe the actual target layout rather than assuming common element sizes or strides.

Use one shared semantic debug description with target-specific serialization. Emit debug metadata after code generation and optimization so code offsets and variable locations match the final instructions. Keep source Debug/Release options separate from whether the compiler executable itself was built in Debug/Release.

Preserve the existing language and array decisions. Debugger display does not change array initialization, bounds, string terminator or capacity rules.

## Target implementation and qualification

| Target | Required implementation | Acceptance evidence |
| --- | --- | --- |
| Windows x64 | CodeView types and scoped variable locations, source records, PDB linking, valid `.pdata`/`.xdata` unwind information | Execute linked fixtures under CDB or another Windows debugger; verify breakpoints, element values, scopes and caller frames |
| Linux x64 | Native ELF pipeline, DWARF types/locations/source records and call-frame information | Execute fixtures under GDB/LLDB on Linux |
| macOS ARM64 | Native Mach-O pipeline, DWARF/dSYM integration and ABI-correct frame unwinding | Execute fixtures under LLDB on macOS |
| TMS6747 | Actual DSP type/layout descriptions, variable locations and source records consumable by the selected toolchain/debugger | Verify TI/CCS behavior separately from the bundled assembler/linker; qualify source stops, array values and caller frames |

Linux and macOS currently have frontend-only target selections. Their debugger requirements cannot be closed until native code generation and linking exist. TI source-debug evidence does not qualify the bundled DSP toolchain, which currently lacks DWARF emission. Windows line records do not establish array value debugging or stack-unwind correctness.

## Repair sequence

1. Establish executable debugger fixtures and record the current failures. Start on Windows, where a native debugger and linker are available.
2. Describe implemented local integer arrays using real array types, extents, frame offsets and live ranges. Add parameter and scalar locations without treating arrays as scalars.
3. Supply Windows unwind information for every generated function that changes the stack or calls another function. Validate stops inside checked runtime failures as well as ordinary returns.
4. Preserve source columns, variable identities and lexical scopes through lowering. Validate same-name locals in nested scopes and separate functions.
5. Describe DSP array storage faithfully, including the current element stride. Validate TI metadata and CCS inspection, then implement or explicitly gate the bundled-tool debug path.
6. Implement Linux/macOS native object, ABI and runtime pipelines; qualify their DWARF and debugger behavior on the corresponding operating systems.
7. Extend the same checks whenever remaining language/backend coverage is implemented, including other array element types, multidimensional/global arrays and array parameters.

Each step closes only when executable debugger evidence passes. Object inspection is a useful intermediate check, but does not replace debugger execution. Native runtime tests remain necessary to guard against code-generation regressions.

## Release gate

Maintain a matrix of target, language construct, source mode, toolchain, debugger and observed result. Record unsupported code generation separately from failed debugging. Do not declare four-target completion while a target lacks its native pipeline or while only metadata emission has been tested.

Mixed-language debugging also requires an explicit calling convention, linkage naming, type representation and ownership contract at language boundaries. The other two languages are not identified in this workspace requirement; no compatibility with them is claimed. Shalimar's own debug work proceeds independently.
