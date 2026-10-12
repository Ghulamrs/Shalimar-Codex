# Shalimar IR v0.1 — normative design contract (rev24)

Status: **proposed contract, not yet a conforming implementation**. This document distinguishes the required IR from the legacy string-encoded IR in `include/IR.h`.

## Authority and semantic preservation

`docs/SHALIMAR_LANGUAGE.md` and its subsequently approved normative amendments define Shalimar. The IR cannot add user-visible language constructs or redefine language behavior. Any disagreement is a compiler defect or an explicitly tracked specification ambiguity. AST-to-C is an optional regression oracle, never the definition of semantics.

## Typed entities

An IR module contains global declarations and functions. Each function has a symbol identity, ordered input parameters, ordered result types, entry block, and a set of basic blocks. A block contains typed instructions and exactly one terminator. A value has a unique identity and a type: `int`, `real`, `char`, `void` only as zero outputs, or `array(element_type, rank, extents)` with explicit dynamic extents and ownership/lifetime metadata. Strings are character arrays with specified capacity, length and termination semantics. Reference parameters encode borrowing/copy-in/copy-back behavior explicitly. Target ABI lowering must not leak into these signatures.

## Control flow and dominance

Each block ID is unique per function; entry exists; all successor targets exist. Terminators are `br`, `condbr`, `return`, or `unreachable` (only where justified). Terminator edges and successor metadata agree. Every operand use is dominated by its definition (with block parameters/phi nodes for merges). Branch conditions are typed. All reachable paths satisfy function return arity and types. Loop `break` and `continue` lower to ordinary edges with correct scope targets.

## Multi-output values

A function `fun <int, real> = f(...)` has two IR results `(int, real)`. `call f(args)` yields an ordered pair of distinct typed value IDs. `return (v0, v1)` matches the declared result list exactly. Multi-assignment binds each result in order; each call is evaluated exactly once. This is not a Shalimar `struct` or a platform ABI promise. Hidden output parameters may be introduced only during target-specific ABI lowering.

## Arrays, strings and effects

Array indexing requires rank/dimension agreement and explicit checked bounds operations. Storage has an explicit lifetime, capacity and aliasing policy. Loads/stores and calls carry observable effects so optimizers cannot reorder or remove externally visible behavior. Initialization, source-order global effects, output formatting, errors, and runtime checks are preserved. Integer overflow, division by zero, real conversions, and text semantics must follow the normative reference; where unresolved, optimizations must not assume undefined behavior.

## Validation gates

1. Structural verifier: unique function/block IDs, entry, valid CFG edges, terminator discipline, source-line sanity.
2. Typed verifier: operand/result types, definition dominance, calls, returns, memory operations and array metadata.
3. Semantic lowering tests: specification-derived expected outputs, negative tests, and side effects.
4. Optimizer tests: verify before and after each pass, and compare Debug/Release behavior.
5. Native backends: separately verify calling conventions, symbols, relocations, object formats, debug data, and linkability for each platform.

The existing IR uses string opcodes/operands and does not yet encode all of these guarantees. No native backend should treat this IR as trusted until the typed verifier and lowering tests pass.
