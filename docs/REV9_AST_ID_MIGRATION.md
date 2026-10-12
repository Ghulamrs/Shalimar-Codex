# Revision 9: typed index-based AST handles (first migration stage)

This revision is based on Shalimar_rev8_checked_ast_access.zip.

## Implemented
- Added `AstId<T>` as a C++98 value-only handle containing an AST pool number and index, with no raw pointer members.
- Added `AstArena::id(AstRef<T>)` for checked transition from legacy handles; it rejects a handle from a different arena.
- Added `AstArena::node(AstId<T>)` (mutable and const) to resolve a typed node through the owning arena with checked pool/index and type.
- Migrated Parser.cpp's function-existence checks to `Program::findFunctionId()` and `.valid()`.
- Added tests/ast_id_test.cpp to test IDs, deque growth, const access, null IDs, and cross-arena legacy-handle rejection.

## Verification (Linux)
- Strict C++98 build with `-Wall -Wextra -pedantic -Werror`: passed.
- Existing `tests/run-smoke.sh`: passed.
- `tests/ast_id_test.cpp` compiled and executed under strict C++98: passed.

## Remaining migration (not complete)
- `AstRef<T>` and its raw arena pointer and `operator->` remain in most AST fields, parser, checker, optimizer, and backends. This revision **does not** claim complete replacement.
- `AstId<T>` has no embedded arena identity; passing an ID to a different arena can resolve an unrelated node of the same pool/index. A fully safe design needs explicit arena identity/generation validation or a stronger owning-program context.
- The AST currently uses `std::deque` typed pools, not `std::vector`; this preserves stable references while growing. A future vector conversion requires carefully avoiding stale references during insertion.
- Other compiler-side pointers and generated-C runtime allocations remain outside the scope of this stage.

## Next stage
Migrate AST edges and traversal APIs to `AstId<T>` and explicit arena resolution, add arena-identity checks, remove the legacy `AstRef<T>` bridge only after all call sites migrate, then audit remaining pointers and rerun all tests.
