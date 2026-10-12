# Revision 14 — `char[]` return contract and conformance

This phase addresses the first remaining P1 specification discrepancy after revision 13.
The language reference previously both allowed `char[]` returns and stated that no arrays
could be returned. Section 15 now states the exception consistently.

## Contract
- A function may return one `char[]` string; general numeric array returns remain excluded.
- The returned text is an owned copy, valid after local storage is released.
- Assignment to an existing bounded `char[]` copies with truncation and NUL termination.
- The AST-to-C and IR-native backends must produce identical observable output.

## Scope
This revision updates documentation and adds a focused regression test. It does not
claim to complete IR-native backend parity, pointer elimination, or general string libraries.
