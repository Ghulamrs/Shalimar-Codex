# Compiler allocation audit (rev6)

The executable compiler implementation has zero explicit manual allocation/deallocation operations. The repeatable token audit is tests/audit-allocation.py; its machine-readable output is docs/validation/rev6/allocation-audit.json.

The audit excludes comments and C++ string literals from executable compiler tokens and separately identifies the 18 generated-C runtime literals with allocation terms. Thus it does not claim zero textual occurrences of malloc/free or a container-based generated C runtime. AST pool ownership is automatic; resolver pointers are non-owning observers. See REV6_TEST_RESULTS.md for scope, lifetime rules, sanitizer limits, and remaining work.
