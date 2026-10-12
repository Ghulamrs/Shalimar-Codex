# rev37: restricted Windows x64 argument passing

The S-native backend now supports direct calls with zero arguments or one integer literal argument. The argument is loaded into ECX under the Windows x64 ABI, with the existing 40-byte caller frame for shadow space and alignment. COFF REL32 relocations are emitted for the target symbol. The callee must be defined in the same compilation unit and return one integer; a one-parameter callee can currently return a constant, but cannot read its argument. The compiler rejects other argument forms.

Strict C++98 build and `tests/run-rev37-native-argument.sh` are the acceptance gate. Object inspection confirms the expected instruction bytes and relocation, but native Windows linking/execution and debugging have not yet been tested. C generation remains removed.
