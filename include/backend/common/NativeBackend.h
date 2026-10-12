#ifndef SHALIMAR_NATIVE_BACKEND_H
#define SHALIMAR_NATIVE_BACKEND_H
#include "IR.h"
#include <string>
namespace shalimar {
/* C++98 non-owning backend contract; no C-source fallback is permitted. */
struct NativeTarget {
    std::string architecture;
    std::string objectFormat;
    std::string debugFormat;
};
struct NativeOptions {
    bool releaseMode;
    bool debugSymbols;
    NativeOptions() : releaseMode(false), debugSymbols(false) {}
};
class NativeBackend {
public:
    virtual ~NativeBackend() {}
    virtual bool emitObject(const IRProgram& program, const NativeTarget& target,
                            const NativeOptions& options, const std::string& path,
                            std::string& error) const = 0;
};
/* rev29 intentionally refuses object emission until implemented and verified. */
class UnimplementedNativeBackend : public NativeBackend {
public:
    virtual bool emitObject(const IRProgram& program, const NativeTarget& target,
                            const NativeOptions& options, const std::string& path,
                            std::string& error) const;
};
}
#endif
