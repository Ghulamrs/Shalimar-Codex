# rev17 — specification example regression integration

The 12 original `.shm` examples from `test_shalimar.zip` are copied unchanged into `tests/spec-examples/`.

## Verified on Linux

- Existing rev16 C++98 compiler source: CMake build PASS.
- Original example syntax and semantic checks: 12/12 PASS.
- Generated C compilation with the host C99 compiler in Debug and Release: 12/12 PASS in each configuration (24 object compilations).
- Existing `tests/run-smoke.sh`: PASS.

Run `SHALIMAR_COMPILER=/path/to/shalimarc sh tests/run-spec-examples.sh`.

## Limitations

The examples were not all executed with validated expected-output or numerical-accuracy oracles. IR-native parity for these 12 examples, cross-platform builds, and full Shalimar 3.0 conformance are NOT established by this milestone. The parser fixes were already present in the recovered rev16 baseline; rev17 adds durable regression fixtures and an executable regression script rather than claiming to have rewritten the parser.
