# Revision 8 — checked AST access and pointer audit checkpoint

Baseline: revision 7 recovered project.

## Implemented
- Removed `AstRef<T>::get()` returning `T*`.
- `AstRef<T>::value()` now returns `T&` and throws for null or mismatched handles.
- `astCast` uses checked `dynamic_cast<T&>` and catches `std::bad_cast`.
- Converting AST handles validates their typed access with a reference cast.
- Included `<typeinfo>` for portable `std::bad_cast` handling.

## Verification
- Linux strict C++98 build (`-std=c++98 -Wall -Wextra -pedantic -Werror`): passed.
- `sh tests/run-smoke.sh`: passed.

## Remaining, NOT completed
- `AstRef<T>` retains `AstArenaAccess* arena_` and pointer-returning `operator->()`; full index-based redesign remains outstanding.
- Other compiler-source pointers remain in C-library parsing, parameter lookup, command-line interfaces, and generated C runtime templates.
- Generated C uses `malloc`/`free` in supported runtime paths. Removing these requires a separate runtime/back-end redesign.
- No claim of global pointer-free source or full memory safety is made.
- macOS/Windows native builds remain unverified.

## Naming
`AstRef<T>` is a named C++ template, not a syntax token. C++ `&` is a reference operator/declarator and cannot replace nullable, reassignable handles. `AstId<T>` or `NodeId<T>` would be suitable names for a future value-based handle containing an arena identifier, pool identifier and index, with checked reference resolution.
