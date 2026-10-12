# rev51/rev52 — Per-argument return evaluation

## Normative language rule

Each expression in a multiple-return statement is an ordinary Shalimar expression. Evaluate arguments in source order, exactly once, check each value against its corresponding declared return type, and store the resulting scalar into its field of the caller-owned return structure. No constants-only restriction applies. Arrays are updated through reference arguments rather than returned by value.

## Implementation checkpoint

The Windows x86-64 native selector now uses `evaluateConstantArgument` to independently resolve each constant integer SSA expression tree (+, -, *) with a recursion bound of 64. Duplicate SSA definitions, unresolved operands, and unsupported operations fail closed. Existing parameter-dependent integer addition/subtraction and real/char paths are retained. This is not yet the general runtime evaluator: arbitrary parameter-dependent trees, calls, side effects, full multi-assignment, and native executable-level validation remain outstanding.

## ABI target obligations

Windows x64: int EAX, real XMM0, char AL; caller-owned multi-result buffer. Linux x64: int EAX, real XMM0, char AL; caller-owned multi-result buffer under Shalimar's internal ABI. macOS ARM64: int W0, real D0, char low W0, indirect-result pointer X8. TI TMS6747: int A4, real A5:A4, char low A4; verify hidden-result-pointer ABI location before target qualification.

## Verification

Strict C++98 build and all available `tests/run-rev51-*.sh` scripts passed on the current host. This checks generated objects, not runtime execution or cross-platform qualification.
