# rev51: multiple runtime arithmetic return fields

The native Windows x86-64 selector now accepts multiple independent `binary +` and `binary -` IR instructions in a single multi-scalar return, when each instruction uses the sole integer parameter as its left operand and an unsigned 32-bit literal as its right operand. It also retains the prior literal, parameter, and `char` conversion cases.

Regression input: `tests/rev51_multi_runtime_two_expressions.shl`. The Debug and Release COFF disassemblies show `add $0xa,%eax`, `sub $0x7,%eax`, and stores to the two typed result offsets. Strict C++98 build and the rev51 multi-return regression scripts pass.

This is a restricted native-object test, not Windows executable execution. Arbitrary expression trees, general multi-assignment, and the macOS ARM64 / TI TMS6747 backend implementations are still outstanding. ABI conventions are documented separately in the technical manual.
