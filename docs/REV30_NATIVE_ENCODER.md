# rev30: x86-64 native encoder foundation

This revision implements a minimal **direct x86-64 machine instruction encoder** in `src/backend/x86_64/`, independently of the AST-to-C and IR-to-C generators. Supported encodings are push rbp, mov rbp/rsp, pop rbp, mov eax/imm32, add eax/imm32, and ret. Instruction source marks record byte offsets, source filename, line and column for future debug-line table generation.

## Scope and limitations

This is an instruction encoding foundation, **not** a working S-mode object generator. No IR instruction selection, register allocation, symbol resolution, relocations, ELF/COFF/Mach-O writers, DWARF or CodeView/PDB emission is implemented. The encoder is intentionally not wired into the `--middle=s` object emission path; unsupported emission continues to fail rather than falling back to C. Source marks are a prerequisite, not debug symbols. Tests assert exact opcode bytes and source offsets. No manual `new`, `delete`, `malloc`, or `free` is used in the added compiler code.
