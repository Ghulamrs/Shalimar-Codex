# rev51 checkpoint: integer runtime parameter in multi-result callee

The native Windows x64 COFF selector now handles a single incoming by-value
`int` parameter in a multi-result return, including repeated uses alongside
integer and real constants. The hidden caller-owned result pointer is passed
in RCX; the integer parameter is read from EDX. Typed field offsets are
provided by IRReturnLayout. The selector emits direct stores and returns RCX
in RAX. Debug and Release disassembly tests verify this limited pattern.

This is not general multi-return support. Calls and multi-assignments are not
lowered, floating-point runtime inputs and computed result expressions are not
supported by this pattern, and Windows execution has not been verified.
The AstRef migration remains deferred.
