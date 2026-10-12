# Revision 13 — no-output function shorthand

Both `fun main()` and `fun <> = main()` are accepted. The same applies to any no-output function. The EBNF now documents the optional output-list prefix. This revision does not remove legacy `AstRef` or change the `char[]` contract.

Tests: rev13_short_main.shl, rev13_legacy_main.shl, rev13_short_helper.shl, rev13_invalid_value_return.shl.
