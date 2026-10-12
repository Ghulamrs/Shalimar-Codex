# Revision 12 — verified migration checkpoint

This is NOT the finished pointer-free Shalimar compiler.

## Change

`src/IRBuilder.cpp` now resolves function calls through `Program::findFunctionId`, `AstId<FunctionDef>`, and checked `AstArena::node` references. It no longer uses a raw `const Parameter*` for parameter lookup in that path.

## Verification

- Linux CMake build: passed, C++98 standard configured.
- `bash tests/run-smoke.sh`: passed.
- Windows and macOS builds: not verified.

## Remaining work

- `AstRef<T>` remains throughout AST nodes and compiler passes.
- AST arena compatibility layer still contains raw pointer usage.
- Other compiler source files and generated C runtime still contain pointers.
- Full language conformance and target-platform validation remain incomplete.

## AST handle inventory

`AstRef<...>` occurrences in source/header files: include = 123, src = 292.

The finished-package acceptance criteria have NOT been met.
