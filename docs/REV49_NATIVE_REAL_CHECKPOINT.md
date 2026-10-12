# rev49 in-progress checkpoint: native real literal return

Added Windows x64 XMM0 binary64 return lowering for a zero-argument function whose sole instruction returns a finite numeric literal. The code uses existing C++98 instruction encoder primitives and emits no C backend code.

Verification: strict C++98 build; `tests/run-rev49-real-literal.sh` Debug and Release COFF objects; `tests/run-rev45-sqrt-native.sh`; conformance matrix 24 PASS, 28 GAP, 1 RECORDED.

This is NOT final rev49. P0 full native language, P1 Linux ELF, P2 macOS ARM64 Mach-O, P3 TMS6747, Windows execution, and CodeView/PDB remain incomplete. The new direct-literal return does not support general floating-point expressions.
