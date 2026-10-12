# rev41 — Native library signature classification

S-mode only. `uses` declarations now carry an IR semantic category and argument descriptor list.

- External real-valued math: arguments and result are `real` in the import contract.
- Polymorphic `abs`, `min`, `max`: result is explicitly `unresolved` until a typed call is analyzed.
- `len`: compiler intrinsic, array-or-string input, `int` result; **not** an external C library call.

This prevents pretending polymorphic functions have a single `real` ABI. It does **not** implement per-call type resolution, Windows x64 floating-point ABI, native external call emission, executable linking, or debug symbols.

The verifier checks descriptor count, kind and result-category consistency.
