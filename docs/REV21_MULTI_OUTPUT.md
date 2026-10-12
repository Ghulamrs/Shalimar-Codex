# rev21 — multi-output ABI blocker checkpoint

The C++98 compiler rebuild and smoke suite pass. The original 12-program backend parity check remains 6/12; no runtime parity improvement is claimed.

The IR builder emits `multi.assign` and comma-delimited `return` operands, but the IR-native backend has no ABI representation for multiple return values. The default AST-to-C backend emits a `ShalimarResult_<function>` structure, while IR-native's function signatures, call lowering, multi-assign lowering, and return lowering currently assume zero or one return value. Removing the initial rejection without implementing all four layers would generate invalid or incorrect code.

This revision improves the diagnostic at the first unsupported multi-output function boundary. The multi-output feature itself remains unfinished. The source test programs were not modified.

Verified on Linux: C++98 CMake build PASS, smoke suite PASS, exact runtime parity 6/12. Windows/macOS unverified.
