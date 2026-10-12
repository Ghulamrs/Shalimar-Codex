# rev52 — Native typed trailing zero initialization

The IR builder pads omitted trailing multiple-return fields with typed zero operands (`0`, `0.0`, `char(0)`). The restricted Windows x64 IR selector now recognizes the synthesized `char(0)` and emits a zero byte store. Integer and real zeros use their existing 32-bit and 64-bit field stores. The `return ()` form zeroes all fields.

Regression: `tests/run-rev52-trailing-zero-native.sh` verifies Debug and Release COFF disassembly for the `partial` and `empty` functions. These checks are **not native executable runtime tests**. General expressions, multi-assignment and complete cross-platform support remain open. Function calls within return parentheses remain forbidden by the semantic checker.
