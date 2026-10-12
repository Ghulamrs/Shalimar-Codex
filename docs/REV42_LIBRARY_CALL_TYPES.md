# rev42: Typed imported call-site descriptors

The IR now carries `IRLibraryCall` entries for borrowed function invocations,
including argument type descriptors, result type, result SSA name and source line.
Literal scalar arguments are classified as `int` or `real`; complex expressions
are deliberately `unresolved`, rather than guessed. Polymorphic results are
resolved only when all argument types agree. The IR verifier rejects missing
imports and malformed descriptors.

**Scope:** This is call-site semantic metadata, NOT native ABI lowering.
Windows XMM argument registers, external symbol linkage and executable execution
remain pending. `--object` continues to reject imported library calls.
C++98 only; S mode only; no C fallback.
