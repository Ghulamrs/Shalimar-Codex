# rev48: Native source-offset mapping foundation

The `--debug-map PATH` option, valid only with `--object`, writes a versioned TSV manifest of native machine-code offsets and their Shalimar source positions. Offsets are relative to the concatenated COFF `.text` section. This is a **diagnostic artifact, not CodeView, PDB, or source-level debugger support**. The compiler continues to reject `--debug-symbols` until native debug sections are implemented.

The mapping uses the encoder's source marks, not C code. Only instructions explicitly marked by current lowering are present; it is incomplete by design. Debug and Release maps may differ. Existing four-target backends remain untouched.
