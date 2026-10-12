# Independent Shalimar C6747 tool set

These are the user's tools, kept separate from TI's installations:

```
bin/asm6x.exe
bin/lnk6x.exe
bin/vm6747.exe
bin/sim6747.exe
lib/rts6x.lib
lib/rts6xd.lib
```

`manifest.json` records the original files and SHA-256 checksums. No TI tools or TI runtime are bundled here.

The original executables remain at `C:\Users\GRA\source`. The matching runtime was found at `C:\Users\GRA\Developer\Claude\M10\RTS6x\build`. Source projects remain unchanged under `C:\Users\GRA\Developer\Claude\M10`: `ASM6x`, `LNK6x`, `RTS6x`, `SIM6747`, and `vm6747\Emulator`. Those separate source projects are not included in this compiler archive.

From the compiler project root:

```powershell
.\build\vs2022\Debug\shalimarc.exe tests\tms6747\return_42.shl --target=tms6747 --toolchain=shalimar --tools-root toolsets\shalimar-c6747 --dsp-entry answer -o answer.out
.\toolsets\shalimar-c6747\bin\sim6747.exe --run answer.out --main-status -c
```

The compiler accepts either this `bin/lib` layout or a flat directory containing the executables. For a flat directory, place `rts6x.lib` beside the executables or select it with `--rts-lib PATH`. `SHALIMAR_C6747_TOOLS` supplies the default tool root; otherwise the current local default is `C:\Users\GRA\source`.

`vm6747` interprets assembly. `sim6747 --run` decodes the linked machine executable. They are different validation paths. The compiler builds the executable; test scripts launch the simulators separately.

Own-tool Debug mode disables IR optimization but currently has no DWARF source information. An explicit `--debug-symbols` request fails with an explanatory diagnostic. Use the independently selected `--toolchain=ti` path for the presently verified source debugger support.
