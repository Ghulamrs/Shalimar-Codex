# Revision 11: typed AST IDs (incremental checkpoint)

Base: revision 10 archive. The new `AstArena::addId(...)` overloads for all 20 AST node types return `AstId<T>` directly, without requiring a legacy `AstRef<T>` variable at the call site. `AstArena::castId<T>(id)` provides checked type conversion using C++98 reference casts, returning an invalid ID on type mismatch and rejecting cross-arena IDs. The AST ID test exercises construction, conversion, failed downcast, cross-arena rejection and growth stability.

## Validation on Linux

- `cmake -S . -B /tmp/shalimar_rev11_build` and `cmake --build /tmp/shalimar_rev11_build -j 4`: passed.
- `g++ -std=c++98 -Wall -Wextra -pedantic -Iinclude tests/ast_id_test.cpp -o /tmp/shalimar_rev11_idtest && /tmp/shalimar_rev11_idtest`: passed.
- `bash tests/run-smoke.sh`: passed.
- Windows and macOS builds not tested.

## Remaining work — NOT complete

- `AstRef<T>` remains in AST fields and throughout parser, semantic checker, optimizer and code generators. `addId` internally bridges to the legacy `add`, so the old handle system has not been removed.
- Raw pointers remain in compiler code and emitted C runtime code. No claim of pointer-free completion.
- The full language conformance suite has not been rerun in this checkpoint.
- Do not treat this as the final user-requested package.
