# rev46 — Windows native link validation

This revision adds `tests/run-rev46-windows-link.sh` to validate linking of the existing S-native COFF output.

## Verified

- Debug and Release `root` objects generated from `tests/rev45_sqrt_literal.shl`.
- An independent Windows x64 assembly test shim defines `sqrt` using the CPU's scalar `sqrtsd` instruction.
- `lld-link` resolves the COFF `REL32` external relocation and emits a PE32+ Windows DLL exporting `root`.
- Linking fails when the required `sqrt` definition is deliberately omitted.
- The test does not invoke AST-to-C, a C compiler, or any C backend.

## Scope limits

This is a *link-only test*. The generated DLL has not been executed on Windows; debugger symbol emission and native stepping are not implemented. The test shim is not a production runtime library and does not establish compatibility with Windows CRT import libraries. No executable entry point or end-to-end Shalimar program execution is claimed. General floating-point parameters, multi-output functions, and other target backends remain unfinished.

## Reproduce

`tests/run-rev46-windows-link.sh /path/to/shalimarc`

Requires `clang`, `lld-link`, `file`, and `objdump` on the test host.
