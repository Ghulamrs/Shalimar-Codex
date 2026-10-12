# Revision 43 — library call type propagation

S-mode now resolves scalar `int` and `real` library-call argument types from function parameter signatures and preceding scalar declarations within a function, as well as numeric literals. It tracks the result type of recognized library calls. IR dump exposes resolved library-call signatures.

## Limits

This is intentionally a narrow, linear pass. It is not a complete control-flow-aware type analysis: assignments, merges, complex expressions, arrays and overload resolution need further work. The Windows x64 XMM encoder, external library call ABI, native linker execution, and debugging symbols are not implemented by this revision. No AST-to-C fallback is used.

## Verification

Run `tests/run-rev43-type-propagation.sh <shalimarc>` after a strict C++98 build. It checks both Debug and Release.
