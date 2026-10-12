# rev49 P0: Checked non-template AST narrowing

Added 19 named `as*` conversions from polymorphic ExprId/StmtId to concrete IDs. All conversions verify arena ownership, bounds and node kind before returning a typed value ID. This enables future parser/semantic/IR migration without `astCast<T>`. Existing `AstRef<T>` and `astCast<T>` remain in production code and are not claimed removed.
