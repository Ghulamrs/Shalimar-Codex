# Multiple-return trailing zero initialization

Normative rule: a parenthesized return list may contain fewer expressions than the function's declared return outputs. Supplied expressions map to the leading output positions, are type checked, and are evaluated left to right. Every omitted trailing output is zero-initialized according to its declared type (`int`: `0`, `real`: `0.0`, `char`: `char(0)`). `return ()` initializes all outputs. Excess outputs are a semantic error. Calls within parenthesized return expressions remain forbidden.

Implementation: `Parser.cpp` accepts `return ()`; `SemanticChecker.cpp` accepts at most the declared number of values; `IRBuilder.cpp` appends typed zero operands to the `return` instruction.

Verification: strict C++98 compiler build; `tests/return_trailing_zero_valid.shl` produces expected IR; `tests/return_excess_invalid.shl` rejected. Existing rev51 char caller test uses `char(65)` inside `return (...)` and now fails semantic validation because it violates the finalized no-calls rule. This legacy test needs an updated fixture, not relaxation of the new rule. Native object generation for the new mixed typed return example remains unsupported by the current restricted Windows x64 selector; executable verification pending.
