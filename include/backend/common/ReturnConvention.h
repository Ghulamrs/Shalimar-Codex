#ifndef SHALIMAR_RETURN_CONVENTION_H
#define SHALIMAR_RETURN_CONVENTION_H

#include <cstddef>
#include <string>

namespace shalimar {

/* Physical register names describe ABI boundaries, not temporary allocation.
   Multi-result values always use caller-owned storage. Arrays are in/out args. */
struct ReturnConvention {
    std::string target;
    std::string integerRegister;
    std::string realRegister;
    std::string charRegister;
    std::string indirectResultRegister;
    bool indirectResultInFirstArgument;
    bool verifiedNativeImplementation;
};

inline ReturnConvention returnConvention(const std::string& target) {
    ReturnConvention r;
    r.target = target;
    r.indirectResultInFirstArgument = false;
    r.verifiedNativeImplementation = false;
    if (target == "windows-x86_64") {
        r.integerRegister = "EAX";
        r.realRegister = "XMM0";
        r.charRegister = "AL";
        r.indirectResultRegister = "RCX";
        r.indirectResultInFirstArgument = true;
        r.verifiedNativeImplementation = true; // Restricted rev51 patterns only.
    } else if (target == "linux-x86_64") {
        r.integerRegister = "EAX";
        r.realRegister = "XMM0";
        r.charRegister = "AL";
        r.indirectResultRegister = "RDI";
        r.indirectResultInFirstArgument = true;
    } else if (target == "macos-arm64") {
        r.integerRegister = "W0";
        r.realRegister = "D0";
        r.charRegister = "W0[7:0]";
        r.indirectResultRegister = "X8";
    } else if (target == "tms6747") {
        r.integerRegister = "A4";
        r.realRegister = "A5:A4";
        r.charRegister = "A4[7:0]";
        r.indirectResultRegister = "ABI_UNVERIFIED";
    }
    return r;
}

inline bool hasReturnConvention(const ReturnConvention& r) {
    return !r.integerRegister.empty() && !r.realRegister.empty()
        && !r.charRegister.empty() && !r.indirectResultRegister.empty();
}

/* The number of scalar results is a language-level property. */
inline bool usesStructuredReturn(std::size_t scalarResultCount) {
    return scalarResultCount > 1;
}

} // namespace shalimar
#endif
