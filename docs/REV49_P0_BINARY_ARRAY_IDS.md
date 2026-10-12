# rev49 P0 binary and array literal IDs

Parser binary-expression and array-literal construction now starts from named, non-template `BinaryExprId` and `ArrayLiteralExprId` handles. Array children are inserted through checked `arena.node(arrayId)` access. The `AstRef<T>` compatibility conversion remains until consumers are migrated. This is not full P0 completion.
