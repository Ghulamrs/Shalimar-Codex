# rev33 — Windows x64 COFF object foundation

`--middle=s --object output.obj` now generates a genuine AMD64 COFF relocatable object for the restricted zero-argument, single-`int`-return constant-function subset. It uses Shalimar IR -> x86-64 encoder -> binary COFF writer; **no C generation**.

The COFF writer emits `.text`, a symbol table, and long-name string table. No relocations are emitted because this subset does not support calls or data references. Debug symbols, stack unwinding, Windows ABI parameters, general control flow, multi-output, and full Shalimar programs are NOT supported. `--debug-symbols` is rejected explicitly.

The COFF object is suitable for inspection with `llvm-readobj`/`objdump` and for linking by a compatible Windows linker when the function subset is supported. Windows linker execution and debugger validation are still pending.

All active backend code must remain strict C++98, without manual owning allocations. Legacy four-target code remains present for further migration. The TI TMS6747 remains a required target, not a complete backend.
