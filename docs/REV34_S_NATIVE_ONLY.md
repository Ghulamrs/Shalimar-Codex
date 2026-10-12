# rev34: S-native-only compiler

The compiler no longer offers C mode, `--emit-c`, `--ir-native`, or `--cc`.
The C code generators are excluded from CMake and removed from active source.
`--middle=s` is accepted as an optional compatibility spelling.

## Status

The S frontend supports `--check`, `--dump-ir`, and restricted Windows x64 COFF `--object`.
Object output currently supports constant-return functions only. There is no C fallback.
Native debug symbols, relocations, full ABI, other object formats, and DSP output remain unfinished.
Legacy C-related tests may remain as historical fixtures but are not evidence of S-native functionality.
