# rev51 return convention — frozen design, implementation not yet complete

* Zero scalar returns: no return value.
* One `int` / `char` / `real`: the target ABI's scalar return register.
* Two or more scalar returns: a typed, aligned, caller-owned result buffer.
* Arrays: never returned by value; updates flow through reference parameters.
* Temporary register assignment remains free, but ABI boundary locations are fixed.

| Target | int | char | real | structured return address |
|---|---|---|---|---|
| Windows x64 | EAX | AL | XMM0 | RCX (hidden first argument) |
| Linux x64 SysV | EAX | AL | XMM0 | RDI (hidden first argument) |
| macOS ARM64 | W0 | low byte of W0 | D0 | X8 (indirect result) |
| TI C674x | A4 | low byte of A4 | A5:A4 | **Not verified; must follow selected TI ABI** |

The mapping is a target ABI contract and must not be confused with implemented code generation. Current rev51 Windows native selection only supports a restricted subset. Linux, macOS and DSP code generation and executable testing remain open. The C674x indirect return address location must be verified against the chosen TI ABI; do not invent a register assignment.

See `TECHNICAL_MANUAL_RETURN_ABI.md` for the permanent technical-manual section, testing requirements, and qualification boundaries.
