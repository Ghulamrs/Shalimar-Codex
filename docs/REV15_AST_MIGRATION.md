# Revision 15 — non-template function identifiers

This is an incremental, regression-tested checkpoint, not the completed AST migration.

## Changes

- Introduced `FunctionId`, a non-template value type carrying an arena identity and function-pool index.
- Introduced `AstArena::functionId()` and checked `AstArena::function()` reference accessors.
- Introduced `Program::findNamedFunction()`.
- Migrated IRBuilder function-call parameter lookup away from `AstId<FunctionDef>`.
- Retained all standard-library container templates, per project design policy.

## Validation

- Linux CMake build using C++98: passed.
- `tests/run-smoke.sh` on the new compiler: passed (including AST and IR-native smoke cases).
- A repository-wide `AstRef`/`astCast` removal is **not** complete.
- This revision does not establish Windows/macOS compatibility or full memory safety.

## Next migration

Replace AST relationships and parser/semantic-checker operations with non-template
`ExprId`, `StmtId`, and named accessors. Do not remove the legacy bridge until all
callers are migrated and regression-tested.
