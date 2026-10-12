# rev51 real-valued caller retrieval checkpoint

Windows x64 S-native multiple-return caller lowering now supports retrieving a `real` result from the caller-owned structured result buffer. The selector emits `MOVSD XMM0, qword ptr [RSP+offset]` after the callee returns and before the stack frame is released. Integer retrieval continues to use EAX. The fixed hidden-pointer structure convention is unchanged.

Test: `tests/run-rev51-multi-real-caller.sh` checks frontend acceptance and Debug/Release COFF disassembly/relocation. This is **not** end-to-end executable qualification. General runtime expressions, arbitrary multi-assign use, and target execution remain incomplete. rev51 is still in progress.
