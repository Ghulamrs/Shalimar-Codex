# rev44: Windows x64 binary64 XMM encoder groundwork

This revision adds direct x86-64 encoding for loading an IEEE-754 binary64
constant into XMM0 using RAX and MOVQ, moving XMM0 to XMM1 with MOVSD,
and adding packed-scalar double precision via ADDSD. This is independent of
C generation and uses only C++98 in the compiler.

## Acceptance

Run `tests/run-rev44-xmm.sh`. It checks exact instruction bytes for 3.5.
Existing rev43 library type propagation and Windows COFF regressions remain
required.

## Limitations

These encodings are not yet wired into the IR selector or native library-call
lowering. There is no Windows `sqrt` executable or XMM ABI integration yet.
Windows linking, CodeView/PDB debugging, and all-target native execution
remain unverified.
