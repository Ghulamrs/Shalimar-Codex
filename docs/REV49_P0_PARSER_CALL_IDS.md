# rev49 P0: parser call-expression IDs

The parser now constructs call expressions using the named non-template `CallExprId`, and appends arguments through checked `arena.node(call)` access. A checked `legacyCallRef` bridge remains only because AST child fields and parser return types still use `AstRef<T>`. This is incremental, **not** completion of P0. No new/delete or malloc/free added.
