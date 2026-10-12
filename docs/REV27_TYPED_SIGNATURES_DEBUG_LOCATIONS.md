# rev27: typed function signatures and source metadata

S-mode IR functions now carry typed input and output signatures, including multiple return types, array ranks and by-reference input flags. Source file and source line information is propagated from the compiler driver through IR lowering into function and instruction debug metadata. The IR verifier rejects invalid signatures and missing source files for located functions/instructions.

This is a partial typed-IR migration: instruction operands are still strings; no machine-code backend, native debugging symbols, register location lists, or complete type-flow verification exists. The C path remains independent; S mode never falls back to C.
