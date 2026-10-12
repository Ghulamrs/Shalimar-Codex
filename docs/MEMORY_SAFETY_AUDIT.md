> rev6 update: this historical document must be read with [REV6_TEST_RESULTS.md](REV6_TEST_RESULTS.md), which is authoritative for current tested status and limitations.

# C++98 Compiler Memory-Safety Audit

**Audit baseline:** 2026-10-11

## Objective

Reduce memory-fault risk without changing the requested C++98 language standard or changing Shalimar program behavior. The target is explicit ownership and reliable cleanup, not a blanket ban on every pointer. Non-owning pointers can be appropriate when object lifetimes are clear; ambiguous ownership and manual lifetime management are the primary risks.

## Findings from the current source

### 1. AST ownership is implemented with raw owning pointers

`include/Ast.h` defines AST nodes such as `IndexExpr`, `BinaryExpr`, `CallExpr`, `BlockStmt`, `DeclStmt`, `AssignStmt`, `IfStmt`, `WhileStmt`, `ForStmt`, `FunctionDef`, and `Program` with raw pointer members or vectors of raw pointers. Their destructors recursively delete owned children.

This gives the tree a recognizable ownership model, but it depends on every child having exactly one owner. Accidental copying of an owning node, inserting the same child twice, or forgetting to transfer ownership can cause double deletion, dangling references, or leaks. The first hardening change introduces a C++98 `NonCopyable` base for expression nodes, statement nodes, `FunctionDef`, and `Program`, preventing accidental shallow copies at compile time. This is a guardrail, not a complete solution for parser exception safety or duplicate-child ownership.

### 2. Parser construction manually cleans up partial trees

`src/Parser.cpp` contains many `new` and `delete` operations across parse success and error paths. This can work, but every early return must release all nodes already constructed. If a standard-library allocation throws while a raw pointer is temporarily owned by a local variable, that object may leak because C++98 does not automatically destroy raw pointers.

Recommended direction: use small C++98 RAII owner helpers during construction, then explicitly transfer ownership only when a node is successfully attached to its parent. Do not attempt a broad parser rewrite in one change; convert one expression/statement family at a time and run malformed-input tests after each step.

### 3. Optimizer replacement had an exception-safety hazard

`src/Optimizer.cpp` replaced a literal by deleting the original expression before allocating the replacement. If allocation failed, the caller's expression pointer could still point at freed storage. This audit fixes the order: allocate the replacement first, then delete the old expression and publish the new pointer. This preserves the original tree if allocation throws before the replacement is created.

### 4. IR data is predominantly value-based

`include/IR.h` stores instructions and basic blocks in `std::vector` values and stores most IR operands as `std::string`. This is a useful design to preserve: prefer value semantics for IR instructions, metadata, and strings rather than adding unnecessary owning pointers.

### 5. Generated C has a separate ownership model

Compiler memory safety does not guarantee generated-program memory safety. The generated C helpers for dynamic string concatenation, returned strings, local arrays, and string copying need their own lifetime/capacity review. In particular, return values must not point into freed local storage, destination copies must reserve a NUL byte, and temporary allocations must have a defined cleanup policy on all exits.

## Prioritized remediation plan

1. **P0 — Ownership invariants:** document which AST pointer members own their children and make owning node types non-copyable unless a deep copy is deliberately provided. (The non-copyable base is now in place.)
2. **P0 — Exception-safe construction:** introduce a small C++98 scoped-owner abstraction for parser locals and transfer ownership only after successful attachment.
3. **P1 — Container ownership:** consider an owning AST arena or owner collection for nodes, with non-owning child links. Choose one model and apply it consistently; do not mix arena ownership and recursive deletion.
4. **P1 — Failure-path tests:** test malformed/truncated source, duplicate declarations, invalid nested expressions, and repeated compilation under AddressSanitizer/UndefinedBehaviorSanitizer where supported.
5. **P1 — Generated C lifetime review:** test returns from local arrays, concatenation, self-append, global initializers, bounds failures, and early exits.
6. **P2 — Allocation-failure behavior:** where practical, ensure `std::bad_alloc` is reported cleanly at the compiler boundary without continuing with partial state.
7. **P2 — Document exceptions:** keep pointer use for non-owning references where appropriate, and confine any remaining explicit allocation/deallocation to audited ownership code.

## Rules for follow-up changes

- Do not mechanically replace every pointer with a reference; references cannot represent nullable children and do not by themselves solve lifetime problems.
- Do not use `std::auto_ptr` as a general ownership system. Its transfer-on-copy semantics are fragile with containers and ordinary function calls.
- Keep C++98 compatibility: do not introduce `unique_ptr`, `shared_ptr`, move semantics, or other later-standard features.
- Every ownership refactor must preserve diagnostics and run the complete regression suite.
- Treat sanitizer results as evidence for exercised paths, not proof that all memory errors are impossible.

## Current status

The audit identified concrete risk areas and one exception-safety issue in optimizer replacement, which has been corrected. AST expression/statement nodes, functions, and programs now reject accidental copying at compile time. This document is not a claim that all raw owning pointers have been removed or that the compiler is now fully memory-safe. Parser construction still requires scoped ownership guards, and generated C lifetime rules need separate verification.
