# Revision 10: AST ID safety checkpoint

This is **not** a completed AstRef-to-AstId migration.

Changes:
- AstId now includes a value-only arena identity, not just a pool and index.
- Only AstArena can construct non-null IDs; arbitrary unbound IDs are no longer public.
- AstArena::node rejects IDs from another arena and invalid/null IDs.
- A new regression checks that IDs from one Program cannot be resolved in another.

Verification performed on Linux:
- Strict C++98 CMake build with -Wall -Wextra -pedantic -Werror: PASS.
- tests/ast_id_test.cpp standalone test: PASS.
- tests/run-smoke.sh: PASS.

Still outstanding:
- AstRef<T> remains extensively used throughout AST nodes and compiler passes.
- AstRef stores a raw arena pointer and has pointer-returning operator->.
- Other raw pointers and generated C runtime allocation remain.
- Full migration needs systematic AST relationship and caller rewrites, and further testing.
- macOS Xcode and Windows Visual Studio builds not verified.
