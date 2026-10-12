# rev49 in-progress: real parameter subtraction

Windows x64 selector supports a restricted single-real-parameter function returning `x - finite_real_literal` via SUBSD XMM0,XMM1. This is native instruction generation only, not a complete expression selector. Both Debug and Release COFF object tests pass. The compiler builds with strict C++98.

AST template removal (AstRef<T>, AstId<T>, astCast<T>), full multi-return machine lowering, native runtime execution, and P0-P3 platform conformance remain open. This checkpoint is not a final rev49 release.
