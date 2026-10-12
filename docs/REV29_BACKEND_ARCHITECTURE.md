# rev29 — native backend boundary

`src/backend/` is the reserved implementation home for S-mode native output.

- `common/`: C++98 `NativeBackend` interface, target/options, and explicit unsupported implementation.
- `x86_64/`, `aarch64/`: future instruction encoders and register allocation.
- `coff/`, `elf/`, `macho/`: future native object writers.
- `debug/`: future source-to-machine debug mapping and DWARF/CodeView emission.
- `c_compat/`: reserved for isolating the C compatibility path; the existing AST-to-C files are not yet migrated.

No code generator, object writer, register allocator, or native debugger symbol emitter is claimed implemented. S-mode `--check` and `--dump-ir` retain their existing behavior. Native output is explicitly rejected without a C fallback. The backend contract is non-owning and does not use manual heap allocation.

Follow-up: connect the verified typed IR to concrete architecture encoders, target ABI lowering, object writers, and debugger validation.
