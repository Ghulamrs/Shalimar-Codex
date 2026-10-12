# rev26 — S-mode structural IR verification foundation

This revision adds `IRVerifier` to the C++98 compiler and invokes it on `--middle=s --check` and `--dump-ir` both before and after Release IR optimization.

It checks unique function names, nonempty function bodies, canonical block IDs, valid and nonduplicate successor indices, nonempty opcodes, and consistent nonnegative source-line metadata. It rejects malformed IR without C code generation.

`IRSourceLocation` (file, line, column) and function/instruction metadata fields are introduced as a schema foundation. Existing builder still primarily populates `sourceLine`; file/column and semantic types are **not yet populated or verified**.

**Not complete:** typed value definitions, type checking, dominance, branch/terminator consistency, multi-result signatures, native object emission, and native DWARF/CodeView symbols. The legacy string-operand IR is not yet a typed IR. No C fallback is introduced for S mode.
