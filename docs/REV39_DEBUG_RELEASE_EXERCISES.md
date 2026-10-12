# rev39: S-mode Debug and Release exercises

This revision adds `--diagnostic-stages` and `--optimization-report` to the S-only compiler. Debug performs frontend, semantic, IR verification and restricted COFF object emission without optimization. Release performs IR optimization and post-optimization verification before restricted COFF emission. Both routes were exercised with `tests/rev38_parameter.shl`.

These switches do NOT generate native debugging symbols, a linked Windows executable, or runtime exception reports. `--debug-symbols` still explicitly rejects unsupported CodeView/PDB emission. The optimization report counts IR instructions before and after optimization; an unchanged count does not mean no optimization occurred. Windows linking/execution is not verified in this environment.

Commands:

```
shalimarc tests/rev38_parameter.shl --check --debug --diagnostic-stages
shalimarc tests/rev38_parameter.shl --dump-ir --release --optimization-report
shalimarc tests/rev38_parameter.shl --object debug.obj --debug
shalimarc tests/rev38_parameter.shl --object release.obj --release --optimization-report
```

Next: actual source-to-CodeView line mapping and debugger verification; Windows ABI completeness, link and executable execution; optimizer equivalence tests on supported programs.
