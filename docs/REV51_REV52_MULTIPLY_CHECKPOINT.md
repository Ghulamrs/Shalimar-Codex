# rev51/rev52 constant SSA multiplication checkpoint

The restricted Windows x86-64 native selector now evaluates constant-only `binary *` instructions, including nested constant addition/subtraction feeding multiplication, using 32-bit unsigned modular arithmetic. A dedicated Debug/Release COFF disassembly regression verifies results 20 and 30 in caller-owned multi-return fields.

This does not implement runtime multiplication of parameters, unrestricted SSA trees, general multiple assignment, Windows execution, or rev52 conformance. These remain open. No macOS or TMS6747 execution is claimed.
