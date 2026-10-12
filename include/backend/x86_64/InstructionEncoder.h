#ifndef SHALIMAR_X86_64_INSTRUCTION_ENCODER_H
#define SHALIMAR_X86_64_INSTRUCTION_ENCODER_H
#include <cstddef>
#include <string>
#include <vector>
namespace shalimar {
namespace x86_64 {
/* Machine-code bytes and source locations; no C translation or heap ownership. */
struct SourceMark {
    std::size_t offset;
    std::string file;
    unsigned line;
    unsigned column;
    SourceMark(std::size_t o, const std::string& f, unsigned l, unsigned c)
        : offset(o), file(f), line(l), column(c) {}
};
class InstructionEncoder {
public:
    InstructionEncoder();
    void clear();
    void markSource(const std::string& file, unsigned line, unsigned column);
    void storeEaxAtRbp(int displacement);
    void loadEaxFromRbp(int displacement);
    void storeAlAtRbp(int displacement);
    void loadAlZeroExtendedFromRbp(int displacement);
    void storeXmm0AtRbp(int displacement);
    void loadXmm0FromRbp(int displacement);
    void pushRax();
    void popRcx();
    void addEaxEcx();
    void subEcxEax();
    void imulEaxEcx();
    void pushRbp();
    void movRbpRsp();
    void popRbp();
    void movEaxImmediate(unsigned value);
    void movEcxImmediate(unsigned value);
    void movEaxEcx();
    void addEaxImmediate(unsigned value);
    void subEaxImmediate(unsigned value);
    void multiplyEaxImmediate(unsigned value);
    void movEaxEdx();
    void ret();
    // Windows x64 real argument/result register groundwork (IEEE-754 binary64).
    void movRaxImmediate64(unsigned low, unsigned high);
    void storeEaxAtRcx(unsigned offset);
    void storeEdxAtRcx(unsigned offset);
    void storeDlAtRcx(unsigned offset);
    void storeRaxAtRcx(unsigned offset);
    void storeXmm1AtRcx(unsigned offset);
    void storeAlAtRcx(unsigned offset);
    void movRaxRcx();
    void movqXmm0Rax();
    void movsdXmm1Xmm0();
    void movqXmm1Rax();
    void addsdXmm0Xmm1();
    void subsdXmm1Xmm0();
    void movsdXmm0Xmm1();
    void subsdXmm0Xmm1();
    void allocateStack(unsigned size);
    void releaseStack(unsigned size);
    void leaRcxRsp(unsigned offset);
    void loadEaxFromRsp(unsigned offset);
    void loadAlFromRsp(unsigned offset);
    void loadXmm0FromRsp(unsigned offset);
    void reserveCallFrame();
    void releaseCallFrame();
    unsigned callRel32Placeholder();
    const std::vector<unsigned char>& bytes() const;
    const std::vector<SourceMark>& sourceMarks() const;
private:
    void emit(unsigned char byte);
    void emit32(unsigned value);
    std::vector<unsigned char> code_;
    std::vector<SourceMark> marks_;
};
}
}
#endif
