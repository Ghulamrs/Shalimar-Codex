# rev35 — Windows AMD64 COFF relocation records

The native COFF writer now accepts per-function relocations. Each record identifies the offset of a 4-byte displacement within the function's machine code, a symbol name, and an AMD64 relocation type. This revision deliberately accepts only IMAGE_REL_AMD64_REL32 (0x0004), with validation of displacement bounds and relocation count. It creates symbol entries for undefined external targets, handles defined targets and long names, and writes a COFF relocation table following the .text section.

## Tested
- Strict C++98 build of compiler and relocation harness.
- COFF format, relocation entries and symbol names inspected with GNU objdump.
- Malformed relocation displacement rejected.

## Not implemented
- Shalimar IR call instruction selection and patching call sites.
- Windows x64 argument passing, shadow space, stack alignment, nonvolatile registers.
- End-to-end Windows linker/runtime execution, CodeView/PDB, DSP object writing.

The existing C-mode removal and four-target backend structure remain unchanged. No C fallback.
