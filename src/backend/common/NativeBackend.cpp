#include "backend/common/NativeBackend.h"
namespace shalimar {
bool UnimplementedNativeBackend::emitObject(const IRProgram&, const NativeTarget&,
                                            const NativeOptions& options, const std::string&,
                                            std::string& error) const {
    error = options.debugSymbols
        ? "S-mode native object and debug symbol emission not implemented; no C fallback"
        : "S-mode native object emission not implemented; no C fallback";
    return false;
}
}
