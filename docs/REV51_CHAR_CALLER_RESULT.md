# rev51 checkpoint: char retrieval from multiple returns

Windows x64 S-native lowering now supports a restricted two-instruction callee IR
pattern for `char(65)` within a multiple-return tuple and retrieves a char
field into AL from caller-owned structured storage. This is not general
multi-assignment lowering or execution qualification.

Test: `tests/run-rev51-multi-char-caller.sh` in Debug and Release.
