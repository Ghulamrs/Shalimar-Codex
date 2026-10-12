# Native local integer-vector implementation

Local one-dimensional int arrays now lower to Windows x64 COFF and TMS6747 assembly. This is an initial native subset, not full array-language or debugger qualification.

## Implemented

- Explicit/inferred/default fixed local extents; zero initialization.
- Literal initialization with omitted zero slots and computed integer elements.
- Element reads and writes with runtime lower/upper bounds checks.
- Bounds checking before indexed assignment's value expression.
- Copies into existing storage, including self-copy and independence from the source.
- Numeric vector len, row, col and dim. Missing/negative axes return -1.
- Checked integer arithmetic on element/index/value expressions.
- Runtime diagnostics name the source line and valid range. Windows uses shalimar_runtime.lib; DSP uses the existing diagnostic/exit RTS facilities.

Storage and address scaling are target-specific: Windows stores 4-byte integer elements; DSP uses existing 8-byte stack slots. Neither representation is exposed as an array-parameter ABI in this tranche.

## Qualification

24 source cases ran with Debug and Release host compilers, source Debug and Release, and both native targets: 192/192 passed. Windows objects were linked as DLLs and executed in child processes; DSP programs were assembled/linked and executed with bundled SIM6747. Results cover zero/default arrays, omitted entries, computed initialization/indexing, copy/self-copy/independence, dimension queries, first/last elements, negative/upper-bound errors, unused-read traps and bounds-before-overflow ordering.

Existing native regressions passed: Windows 108/108 executions and DSP 52/52 checks. Frontend acceptance checks passed 144/144; C++ contracts passed 14/14 in both host configurations. Evidence is in the workspace compiler-review/native-int-arrays, compiler-review/repair-native and compiler-review/array-contract. The source regression entry point is tests/run-native-int-arrays.py; pass --compiler-build pointing to the CMake build containing Debug/Release subdirectories and --output pointing to a results directory. It requires the installed Microsoft linker/SDK and bundled DSP tools. The workspace verifier reproduces the recorded local run.

## Explicit limits and remaining work

Each native vector is limited to 512 elements. Windows additionally limits the complete frame to 4000 bytes until stack probing is implemented; DSP limits allocation to 4096 stack slots. Oversized forms fail selection instead of generating unsafe stack accesses.

Windows vector functions currently require one basic block, at most one scalar int input and one scalar int/real output. Calls inside that selection path are unsupported. Real vectors are now implemented; see NATIVE_REAL_ARRAY_QUALIFICATION.md. Char arrays, multidimensional arrays, globals, array-reference parameters, indexed compound assignment and general array interactions remain unsupported.

Windows now supplies int[] types, extents, frame locations and initialized live ranges for this subset. Actual CDB source-breakpoint, array/parameter/local inspection and step-out checks pass; the existing RBP-frame prologue has Windows unwind records and focused qualification. See [Windows debugger evidence and remaining limits](WINDOWS_DEBUGGER_QUALIFICATION.md). Complete Windows debugger parity remains unfinished. DSP array DWARF/debugger inspection is not qualified. No Linux/macOS native array pipeline is implemented.

Initialization/copy is currently unrolled. Passing correctness checks does not imply optimized array-loop code or a target-independent performance result.
