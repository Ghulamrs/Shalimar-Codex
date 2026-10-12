# rev49 P0 parser source-line migration

Removed the custom `withSourceLine<T>` helper from `src/Parser.cpp`. All nine call sites now set the line explicitly on a concrete statement node. This preserves source-location semantics while reducing custom template usage. This does NOT remove `AstRef<T>` from parser signatures, AST edges, semantic checking, or IR building. Full P0 migration remains open.
