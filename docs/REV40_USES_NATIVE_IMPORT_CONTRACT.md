# rev40 — S-native `uses` import contract

The compiler records each source-level `uses` declaration as an explicit `IRLibraryImport` with a symbol name, argument count, result descriptor and native-support status. The IR verifier rejects malformed and duplicate descriptors. This is **metadata groundwork only**, not a fully typed call ABI: `abs`, `min`, `max`, and `len` can have type-dependent results, and `len` requires intrinsic lowering rather than a platform symbol. Such cases must be resolved from semantic types in later revisions.

The S-only Windows COFF object path explicitly refuses imported-library programs until native ABI and linker mapping are implemented. It does not fall back to C. No claims of native library execution, cross-file module loading, or CodeView debugging are made.

## Next

Implement per-call resolved argument/result types and import kinds (intrinsic vs external), Windows x64 floating-point argument registers, external symbol resolution and executable linking tests. Preserve the four native targets and C++98.
