# rev49 native real parameter arithmetic checkpoint

Windows x64 COFF selection now supports `return x + constant` and `return constant + x` for one by-value real parameter and a finite binary64 constant. XMM0 carries the argument and result, while XMM1 receives the constant via RAX. The emitted operation is `ADDSD XMM0, XMM1`.

This is a restricted selector case, **not** general arithmetic or completed P0–P3. The strict C++98 build and Debug/Release object-byte disassembly checks must pass. Runtime execution on Windows remains pending.
