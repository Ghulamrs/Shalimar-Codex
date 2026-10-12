# rev51 checkpoint: real parameter in multi-return callee

Win64 hidden result pointer is RCX; the first explicit real argument occupies the second argument position and arrives in XMM1. The selector now supports `return (x, 7, x)` for a single real parameter x, storing XMM1 directly at aligned result offsets 0 and 16. This is caller-owned memory return, not the proposed private register-first convention.

Verification: strict C++98 build, Debug/Release COFF object disassembly tests (`tests/run-rev51-real-parameter-multi-return.sh`), and prior rev51 integer-parameter and literal-return regressions. Caller-side multi-assignment, mixed runtime arithmetic, execution, private register-first return ABI, and platform-specific debug info remain incomplete. No claim of rev51 completion.
