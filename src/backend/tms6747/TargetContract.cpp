#include "backend/tms6747/TargetContract.h"
namespace shalimar { namespace tms6747 {
NativeTarget target() {
    NativeTarget t;
    t.architecture = "tms6747";
    t.objectFormat = "TI-C6000-COFF-or-ELF-unverified";
    t.debugFormat = "TI-toolchain-debug-unverified";
    return t;
}
}}
