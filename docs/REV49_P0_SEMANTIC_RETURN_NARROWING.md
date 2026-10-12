# rev49 P0: semantic return-flow narrowing

The semantic checker return-flow helper now uses C++98 checked reference casts instead of two `astCast<T>` calls. This preserves existing return/if-else behavior while reducing reliance on the custom template helper. The `AstRef<T>` function signature is still transitional; no claim of full migration.

Verification: strict C++98 build, rev49 return-layout test, real-parameter subtraction native test, conformance baseline (24 PASS / 28 GAP).

The next substantive step is migrating AST child fields and consumer interfaces to named `ExprId`/`StmtId`, not simply replacing more casts.
