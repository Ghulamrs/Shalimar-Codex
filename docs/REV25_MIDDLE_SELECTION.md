# rev25: explicit middle selection and native-debug non-fallback policy

`--middle=c` selects the established AST-to-C path (default).
`--middle=s` selects the Shalimar IR path for `--dump-ir` and `--check`.

S mode **does not** emit native objects or C, and will fail explicitly for `--object` or `--emit-c`. It does not use the legacy `--ir-native` C emitter.

`--debug-symbols` is reserved for future S-native source-level debugging and is not an implementation of DWARF, CodeView, or PDB.

Legacy `--ir-native` remains available for compatibility, but is not S mode.

Next: typed IR, verifier, debug location/scope metadata, machine backend and native debugger tests.
