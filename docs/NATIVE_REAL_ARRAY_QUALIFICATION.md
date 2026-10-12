# Native local real-vector qualification

Local one-dimensional real arrays now lower to Windows x64 and TMS6747. This closes the local numeric-vector element-type gap within the existing backend limits; it does not close all array coverage or four-target qualification.

## Implemented behavior

- Fixed explicit, inferred and default extents; forced zero initialization when no initializer is present.
- Rectangular vector literals, omitted zero elements and computed values.
- Checked element reads/writes, including checking bounds before evaluating the assigned value.
- Copies into existing storage, self-copy and independence from the copied source.
- Real addition, subtraction, multiplication and negation in vector functions; integer-to-real promotion without changing integer arithmetic's overflow checks.
- Numeric len, row, col and dim queries.
- Windows CodeView real-array types, extents, initialized live ranges and frame locations, alongside integer arrays.

Windows real elements occupy eight bytes with eight-byte-aligned local storage and indexed addressing. Windows integer vectors retain four-byte elements. DSP numeric elements use the existing eight-byte slots; real values use both words, and indexed accesses use LDDW/STDW. Real return values use XMM0 on Windows and A5:A4 on DSP.

## Executed tests

`tests/run-native-real-arrays.py --compiler-build PATH --output DIRECTORY` runs 31 source cases against Debug/Release host compilers, source Debug/Release and Windows/TMS6747: 248/248 pass.

Cases cover first/last elements, forced/default zeroes, omitted entries, computed initialization and indexing, integer promotion, real arithmetic, signed zero, scalar locals, copies/self-copy/independence, mixed integer/real arrays, decimal representation, dimension queries, negative/upper bounds, invalid stores and bounds-before-integer-overflow ordering.

Windows linked DLL results are compared byte-for-byte as binary64 in separate child processes. DSP VM6747 checks both returned words exactly; SIM6747 executes the linked binary and checks termination/status and runtime diagnostics. The simulator's real-return process status alone does not establish the exact real value, so that evidence is not presented as an exact SIM6747 value check.

The Windows debugger runner now includes int[3], int[2], real[3] and real[2] in the same function. Its eight CDB scenarios inspect all integer and real elements, a parameter and scalar local at a source breakpoint, obtain a stack, step out and continue correctly. Debugger display uses CodeView's double type for Shalimar real. Its small/large-frame Windows unwind checks remain passing.

Integer-vector regressions pass 192/192; existing Windows runtime checks pass 108/108 and DSP checks pass 52/52. C++ contract checks pass 14/14 for each host configuration. Evidence is recorded in the workspace compiler-review/native-real-arrays, compiler-review/debugger and compiler-review/repair-native.

## Remaining limits

Each vector is limited to 512 elements. Windows additionally limits the complete frame to 4000 bytes until stack probing is implemented, so some otherwise valid vectors are rejected. The DSP slot limit remains 4096.

Windows vector functions still require a single block, no internal calls, at most one scalar int input and one scalar int/real output. General control flow, other signatures, char buffers, multiple dimensions, globals, array parameters/references and indexed compound assignment remain open coverage items. Initialization and copies are unrolled; no array-loop performance claim is made.

DSP array debugger types/locations remain unfinished. Linux and macOS still lack integrated native output pipelines. See LANGUAGE_COVERAGE_WORK_QUEUE.md and DEBUGGER_RELEASE_REQUIREMENTS.md.
