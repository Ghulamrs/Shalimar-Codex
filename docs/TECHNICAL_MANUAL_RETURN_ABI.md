# Shalimar Technical Manual — Native Return ABI

**Status:** Normative compiler design decision for rev51 onward. Target-specific code-generation and runtime qualification are tracked separately; a documented mapping is not evidence of implementation.

## Language-level return rules

1. A function with no scalar results returns no value.
2. A function returning exactly one `int`, `real`, or `char` uses the target's designated scalar return register.
3. A function returning two or more scalar results uses a **typed, aligned caller-owned result structure**, with fields in declaration order. The caller allocates storage; the callee fills the fields. There is no private register-based multi-result convention.
4. Arrays cannot be returned by value. Array outputs are communicated by reference through the original arguments, preserving shape and bounds metadata.
5. Temporary registers may be allocated freely inside a function. Return locations are fixed at the call boundary. External calls must comply with the selected platform ABI.

## ABI boundary register map

| Target | `int` return | `real` return | `char` return | Multi-result indirect address |
| --- | --- | --- | --- | --- |
| Windows x86-64 | `EAX` | `XMM0` | `AL` | `RCX`, hidden first argument |
| Linux x86-64 (System V) | `EAX` | `XMM0` | `AL` | `RDI`, hidden first argument for the Shalimar internal structured-result convention |
| macOS ARM64 | `W0` | `D0` | low 8 bits of `W0` | `X8`, indirect-result location |
| TI TMS6747 / C674x | `A4` (proposed) | `A5:A4` (proposed) | low 8 bits of `A4` (proposed) | **Unverified: must be resolved against the selected TI ABI** |

The TI entries are design candidates, **not qualified ABI facts**. Do not generate DSP calls based on an assumed indirect-result register before ABI verification.

## Structured return layout

The compiler computes field offsets and alignment from the ordered result types. For example, `(int, real, char)` has illustrative offsets `0`, `8`, and `16` and an aligned 24-byte layout under the current 8-byte alignment rule. This is an internal representation only; the programmer sees separate return values.

## Debug and Release invariants

- Debug and Release must use compatible call boundaries for a given target and linkage.
- Debug must preserve source-visible result locations through correct debug information.
- Release may optimize temporary computations and eliminate redundant copies, but must preserve the observable return contract.
- Test direct calls, nested calls, repeated calls, mixed-type results, reference-array updates, and external ABI boundaries in both configurations.

## Implementation and qualification status

As of the rev51 working tree, Windows x64 native code generation handles a restricted subset of structured-return callees and callers. General runtime expressions, multi-assignment, and executable-level verification remain outstanding. Linux, macOS, and TMS6747 native implementations are **not** qualified by this manual. Do not mark rev51 complete until end-to-end tests pass.

**Implementation source:** `include/backend/common/ReturnConvention.h`. **Design checkpoint:** `docs/REV51_RETURN_CONVENTION_FROZEN.md`.
