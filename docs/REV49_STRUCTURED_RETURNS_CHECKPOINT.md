# rev49 checkpoint: ABI-neutral multi-return layout

The language presents multiple typed return values. `IRReturnLayout` computes a
C++98-compatible, naturally aligned field layout for scalar `int`, `real`, and
`char` outputs; it rejects array and reference outputs. For `(int, real, char)`
the offsets are 0, 8, 16 and the padded total is 24 bytes.

**Not yet implemented:** caller-owned result pointer ABI lowering, callee field
stores, caller field loads, and target-specific conventions on all four targets.
This is metadata and a unit-tested layout foundation, not working multi-return
native execution. The existing rev49 P0–P3 gaps remain open.
