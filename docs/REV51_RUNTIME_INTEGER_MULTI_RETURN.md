# rev51: runtime integer arithmetic in structured multiple returns

This checkpoint adds restricted Windows x64 native lowering of an `int` input
parameter plus an integer literal inside a multi-value return. The hidden
result-storage pointer occupies RCX; the integer input arrives in EDX.

Example: `return (x, 3.5, x + 10)` writes fields at offsets 0, 8, and 16.
The computed field is emitted as `mov eax, edx; add eax, 10; mov [rcx+16], eax`.
The caller-owned structure and scalar-register conventions remain unchanged.

Verification: strict C++98 build, Debug/Release COFF disassembly, and existing
rev51 multiple-return regressions. This is not full expression lowering,
caller multi-assignment, runtime execution, or platform qualification.
