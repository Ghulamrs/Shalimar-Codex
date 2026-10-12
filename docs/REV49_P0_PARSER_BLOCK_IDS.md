# rev49 P0: named BlockStmtId parser construction

The parser now creates block statement nodes with `BlockStmtId` and appends statements through checked `arena.node(blockId)` access. `parseBlock()` still returns the legacy `AstRef<BlockStmt>` for compatibility with existing parser and IR consumers; this is **not** completion of P0.

Validation: strict C++98 build, return-layout regression, and Windows COFF floating-point subtraction in Debug and Release.

Next: migrate parser interfaces and AST child storage to non-template IDs, then semantic checker and IR builder, and remove all legacy AST reference templates.
