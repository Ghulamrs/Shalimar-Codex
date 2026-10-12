#ifndef SHALIMAR_COFF_WRITER_H
#define SHALIMAR_COFF_WRITER_H
#include <string>
#include <vector>
namespace shalimar { namespace coff {
struct Relocation {
    unsigned offset;
    std::string symbol;
    unsigned short type;
    Relocation() : offset(0), type(0x0004) {} // IMAGE_REL_AMD64_REL32
};
struct DebugLine {
    unsigned offset;
    unsigned line;
    std::string file;
};
// D3 variable metadata is intentionally separate from CodeView emission.
// A name and source type alone do not establish a valid machine location.
// All fields use C++98 types; locations must be supplied by the selector.
struct DebugVariable {
    enum LocationKind { Unavailable, Register, FrameRelative };
    std::string name;
    std::string typeName;
    LocationKind location;
    unsigned registerCode;
    int frameOffset;
    unsigned liveStart;
    unsigned liveEnd;
    bool hasLiveRange;
    DebugVariable() : location(Unavailable), registerCode(0), frameOffset(0), liveStart(0), liveEnd(0), hasLiveRange(false) {}
};
struct FunctionBytes {
    std::string name;
    std::vector<unsigned char> code;
    std::vector<Relocation> relocations;
    std::vector<DebugLine> debugLines;
    std::vector<DebugVariable> debugVariables;
};
class CoffWriter {
public:
    bool write(const std::vector<FunctionBytes>& functions, const std::string& path, std::string& error, bool codeView = false) const;
};
}}
#endif
