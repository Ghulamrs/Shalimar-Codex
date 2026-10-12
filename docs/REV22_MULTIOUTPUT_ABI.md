# rev22 — Multi-output ABI regression checkpoint

The original `tests/multi_output.shl` is unchanged. The AST-to-C backend compiles and runs the program, producing `3 2`. The experimental IR-native backend explicitly rejects the same program with `IR-native multi-output function ABI and return lowering are not implemented`.

Added `tests/run-rev22-multioutput.py` to preserve the known-good AST runtime behavior and track the IR-native gap. It accepts the documented explicit rejection or successful matching IR-native output, and fails for unexpected failures or incorrect output. **This revision does not implement the multi-output ABI.**

Remaining implementation work: multi-output return ABI representation; function signatures and forward declarations; call-result storage; `multi.assign` lowering; type-correct return emission; aliasing and reference copy-back; integration tests in Debug and Release. Other IR-native feature gaps from rev21 remain.
