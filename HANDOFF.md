# Current developer handover — 12 October 2026

Start with [the complete repaired-Rev3 handover](C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/Rev3/Shalimar_TMS6747_Windows_Merge_Rev2/ShalimarCompiler/docs/DEVELOPER_HANDOVER_2026-10-12.md). It supersedes the implementation-status statements below and covers approved specifications, repairs, current execution/debugger evidence, remaining deficiencies, tool paths, reproduction and next mandates. The earlier checkpoint is retained for history.

# Shalimar TMS6747 Rev3 — 11 October 2026

Read `docs/TMS6747_REV3_QUALIFICATION.md` for the current implementation, commands, evidence and limits. Older reports describe earlier checkpoints.

The user's request was executed: `shalimarc.exe` used the user's ASM6x and LNK6x, linked the user's RTS6x, and generated `build/own-tools-demo/answer.out`. VM6747 ran its generated assembly and returned 42. SIM6747 ran the linked machine executable and returned 42. The own-tool suite passes 48/48 cases, each checked in VM6747, SIM6747 and independently in CCS 5.5.

Own tools are kept separately in `toolsets/shalimar-c6747`; their originals remain in `C:\Users\GRA\source`. The compiler accepts `--toolchain=shalimar --tools-root PATH --rts-lib PATH`. The separate `--toolchain=ti` path supports the presently verified DWARF source debugger. The own assembler's DWARF support is incomplete and explicit debug-symbol requests are rejected.

Both VS2022 host configurations build. The restricted DSP selector emits 21/167 fixtures through each assembler. Windows DLL regressions pass 108/108 and C++ tests pass 26/26. This is a tested scalar subset, not full language conformance. Source/runtime limits are listed in the current qualification report.

After moving the project, regenerate CMake into a fresh directory; the report gives reproducible commands. Original source installations and earlier archives were preserved.
