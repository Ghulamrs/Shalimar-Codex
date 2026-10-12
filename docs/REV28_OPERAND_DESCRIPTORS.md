# rev28 — Structured IR operand descriptors (incremental)

This revision introduces `IROperandKind` and `IROperand` to each instruction's result, first, and second operand. The constructor classifies temporaries, identifiers, numeric literals, quoted strings, composite comma-delimited values, and special encodings. The IR verifier rejects mismatches between these descriptors and the existing legacy operand strings, both before and after Release optimization.

**Scope:** This is an operand *identity* layer, not complete typed IR. `IRType` in function signatures exists from rev27; operand value types are not yet inferred or resolved. Composite operands still require replacement by structured operand vectors. Neither native machine-code emission nor DWARF/CodeView debugging is implemented. No C-mode fallback is used by S-mode verification.

**Verification:** C++98 build, S-mode multi-output Debug/Release checks, standalone descriptor classification and intentional corruption rejection tests.
