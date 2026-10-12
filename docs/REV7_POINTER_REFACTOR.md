# Revision 7: pointer-refactor checkpoint

Source baseline: recovered `Latest/ShalimarCompiler` tree from the partial rev1-to-rev6 recovery archive.

## Completed
- `Parser::arena` changed from a nullable `AstArena*` to a constructor-injected `AstArena&`; parser calls now use `arena.add(...)`.
- `CodeGenerator::functions` changed from a nullable `const Program*` to a constructor-injected `const Program&`; generated output is unchanged in the smoke suite.
- Main driver constructs `Parser(program)` and `CodeGenerator(program)`.

## Validation
- CMake C++98 build: PASS (Linux environment).
- `sh tests/run-smoke.sh`: PASS (Linux environment).

## Not yet completed
- `AstRef<T>` still has `AstArenaAccess*`, and its `get()` / `operator->()` expose pointer-based access. Replacing these requires a carefully tested index/arena handle design that preserves null handles and typed access.
- `char* end` in number-parsing code remains where required by C conversion APIs.
- Generated C runtime includes `malloc` / `free` for dynamic arrays and string temporaries. C++98 `std::vector` cannot simply be substituted into emitted C99 source.
- This is not proof of global memory safety, and native macOS/Windows builds have not been verified.

Note: the recovered upstream archive is incomplete; this revision builds on its recovered Latest tree, not unrecoverable rev3-rev6 archives.
