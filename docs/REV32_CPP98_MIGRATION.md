# rev32: legacy four-target backend C++98 migration

All 14 original legacy backend headers and implementation files have been copied into `src/backend/legacy_port/` and ported to strict C++98. Original C++14 references remain unchanged under `legacy/backend_reference/`.

Conversion includes ordinary enums, constructor initialization, explicit iterators, C++98 numeric formatting, C99-compatible integer typedefs, removal of `override`, `nullptr`, defaulted destructors, and attributes. Shared instruction and optimizer components are included. The migrated implementation is compiled as part of `shalimarc` using CMake's C++98 standard.

`tests/backend_legacy_port_smoke.cpp` checks assembly text emission for Windows x86-64, Linux x86-64, macOS ARM64 and TI TMS6747. These are **smoke tests**, not evidence that the resulting assembly can be assembled, linked or debugged by native target toolchains.

The port does not yet implement an adapter from typed Shalimar IR to these emitters, a Windows COFF writer, debug symbols or DSP binary emission. Existing S-mode native output restrictions remain.
