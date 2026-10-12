# Revision 36: restricted Windows x64 native calls

The Shalimar S-native IR selector now handles a direct return of a zero-argument `int` function call. It emits `sub rsp, 40`, a `call rel32` placeholder with a COFF relocation, `add rsp, 40`, and `ret`. The 40-byte stack reservation provides Windows x64 shadow space and stack alignment for a function entered with the standard stack alignment. Calls are checked against the IR function table: the callee must be defined and have a zero-input, single-int-output signature.

This is a narrow ABI milestone, not full call lowering. Other expressions, arguments, multi-output functions, recursion, indirect calls, and native debug symbols are not supported by this selector. A COFF inspection test passes on Linux; Windows linking, execution, and debugger testing have not been performed. The S backend does not generate C.

Run: `tests/run-rev36-native-calls.sh /path/to/cmake/build`.
