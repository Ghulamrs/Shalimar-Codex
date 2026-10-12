# Array contract clarified by the language owner

These rules supersede conflicting array examples and statements in earlier language documents. They apply to all targets. They describe required behavior, not a claim of native implementation completeness.

## Numeric arrays

Explicit declaration extents are positive integer literals. Variables and arithmetic expressions are errors. Empty brackets infer extents from a rectangular initializer; mixed explicit/inferred extents are permitted if sizes agree. Without an initializer, unspecified extents default to 3. An empty initializer `{}` is ignored.

An initializer must match every dimension exactly. Partial and oversized initializers are errors. Comma-delimited omitted slots, including trailing slots, count as elements and supply zero. Typed all-blank literals use the declared type; untyped all-blank literals default to int. Ragged rows are errors.

Arrays declared without initialization are zero-initialized. Array assignment copies into existing storage and requires matching type, rank and dimensions. Row assignment requires exactly the existing row shape; it cannot resize a row.

Indices are int and zero-based. Assignment evaluates indices and checks bounds before evaluating the assigned value. An invalid index produces an error before any value-expression side effects.

## Character buffers

Capacity counts storage slots; text length excludes the zero terminator. `char S[5] : "hi"` has length 2 and capacity 5. `char S[] : "hi"` infers capacity 3. Text initialization writes the characters and one terminator; remaining slots have unspecified values. Text plus its terminator must fit; it is never silently truncated.

`len(S)` returns text length up to the first zero; `S.row` returns capacity. Indexing and element assignment use capacity bounds. The final slot is writable; element writes do not protect or restore a terminator. The language owner allows garbage printing if writes leave a buffer unterminated; this is not permission to claim a bounded-memory implementation has been qualified.

Without an initializer, all character-buffer slots are zero. Numeric exact-shape initializer rules do not require string content length to equal buffer capacity.

## Current verification boundary

Rev3 parsing and semantic checking have been repaired for literal/inferred extents, rectangularity, exact numeric initializers, known array-copy shapes and literal string capacity. Parameter-dependent shapes and nonliteral string lengths require runtime checks. Local one-dimensional integer and real vectors now have native implementation and execution evidence on Windows/TMS6747; see NATIVE_INT_ARRAY_QUALIFICATION.md and NATIVE_REAL_ARRAY_QUALIFICATION.md. Windows numeric-vector debugger inspection is qualified for the restricted subset. Char buffers, multidimensional/reference/global arrays and complete debugger coverage remain unfinished.
