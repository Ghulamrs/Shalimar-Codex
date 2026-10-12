# rev51 native multiple-return integer subtraction

The restricted Windows x64 multi-result callee now recognizes `x - constant` as well as `x + constant`, where `x` is the single integer input and the expression supplies an integer field of the caller-owned result structure. The selector emits `mov eax, edx`, `sub eax, immediate32`, then stores EAX to the typed field offset relative to RCX. Integer arithmetic wraps at 32 bits at the instruction level; language-level overflow policy remains to be validated.

Regression: `tests/run-rev51-multi-runtime-sub.sh` checks the generated COFF disassembly in Debug and Release. Existing rev51 multiple-return tests continue to pass. No executable-level, Linux, macOS, or TI TMS6747 validation is claimed. General expression lowering and general multi-assignment remain incomplete.

See `docs/TECHNICAL_MANUAL_RETURN_ABI.md` for the frozen target ABI conventions.
