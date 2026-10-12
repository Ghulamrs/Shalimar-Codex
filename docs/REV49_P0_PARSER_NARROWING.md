# rev49 P0 parser checked narrowing checkpoint

Removed the two `astCast` usage sites from `src/Parser.cpp`. Name-expression call parsing and global-declaration recognition now use named `NameExprId` and `DeclStmtId` with checked `AstArena` access. Added arena identity accessor for bridge construction.

This is an incremental bridge: `AstRef<T>` still exists in parser interfaces and AST storage. No claim of full AST migration or native target readiness.
