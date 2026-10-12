# rev49 checkpoint: non-template polymorphic AST edge identifiers

Added `ExprId` and `StmtId`, non-template value identifiers with arena identity and pool index. `AstArena::edge` converts concrete named IDs to heterogeneous expression or statement edge IDs with owner validation; `AstArena::node` performs owner, pool-category, and bounds checks before resolving. Both const and mutable resolution are supported.

This is **infrastructure only**: existing AST child members, parser, semantic checker, and IR builder still use `AstRef<T>` and `astCast<T>`. The next P0 migration must replace those usages without changing parsing or semantics. No new/delete or malloc/free were introduced. This is not P0 completion.
