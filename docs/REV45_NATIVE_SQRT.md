# rev45 — Restricted Win64 native sqrt import

S-only, C++98 native lowering now supports a zero-argument Shalimar function returning `real` whose entire body returns `sqrt(<finite real literal>)`, provided `uses sqrt` resolves an imported `sqrt(real)->real` signature. The code generator emits `movabs` (binary64 bits), `movq xmm0, rax`, Windows x64 call frame, `call rel32` relocation to external `sqrt`, frame restore, and `ret`. A trivial empty void main can be emitted as `ret`.

## Verification

- Strict C++98 compiler build: PASS.
- Debug and Release COFF emission: PASS.
- `objdump` recognizes `pe-x86-64`, XMM0 load, and `IMAGE_REL_AMD64_REL32 sqrt`: PASS.
- Missing `uses sqrt` negative test: PASS.
- rev30-32, rev35-38, rev43-44 regression scripts: PASS.
- Older rev41/rev42 scripts: FAIL because they assert a blanket rejection of *all* imported functions and an exact obsolete error string. Those assertions are no longer correct once the restricted `sqrt` call is implemented. The old tests are preserved unchanged.

## Not yet supported

- Linking the external `sqrt` symbol to a Windows math runtime, executing the result, or Windows native debugging.
- `sqrt` with parameter, local, temporary, or expression arguments; multiple arguments, multi-output, generalized ABI lowering.
- Direct real literals in returns, or general real arithmetic.
- CodeView/PDB, Linux/macOS/TMS6747 native output.

This revision does not use AST-to-C. All other unsupported calls remain errors.
