# rev51 — caller-owned multiple-result storage (partial)

The Windows x64 native selector now recognizes a restricted zero-argument
multi-result call, followed by `multi.assign` and a final return of either
nothing or one `int` field. It derives the result layout from the callee's
IR signature, reserves 32 bytes of Win64 shadow space plus aligned result
storage, passes the buffer address in RCX, emits a REL32 call relocation,
reads the selected integer field when applicable, and restores RSP.

This is not a general lowering of multi-assignment: the current selector
rejects intervening computations and unsupported return types. There is no
Windows execution proof, no private register-first return convention, and no
CodeView/PDB support. The Debug/Release checks verify generated COFF objects,
relocations, and disassembly only.
