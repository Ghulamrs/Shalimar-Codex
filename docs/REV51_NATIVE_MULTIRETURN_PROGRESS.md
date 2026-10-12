# rev51 — Native multiple-return implementation (IN PROGRESS)

## Implemented
- Windows x64 COFF callee lowering for a single basic block with `return` of two or more finite scalar literals (`int`, `real`, and integer-valued `char`).
- Hidden first argument in RCX points to caller-owned return storage. The callee writes at the offsets computed by `IRReturnLayout`, returns the pointer in RAX, and does not allocate memory.
- Both Debug and Release emit actual machine instructions; the regression inspects their disassembly.

## Not yet implemented / verified
- Caller-side allocation and lowering of multiple-return calls and assignments.
- Returning runtime expressions, variables, or parameter values; this selector accepts literal values only.
- Cross-ABI implementation for Linux, macOS, or TMS6747.
- Windows execution and source-level debugging verification.
- Complete language conformance or Debug/Release qualification.

## Regression
`tests/run-rev51-multi-literal.sh /path/to/shalimarc`
