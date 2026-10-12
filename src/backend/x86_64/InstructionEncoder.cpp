#include "backend/x86_64/InstructionEncoder.h"
namespace shalimar {
namespace x86_64 {
InstructionEncoder::InstructionEncoder() {}
void InstructionEncoder::clear() { code_.clear(); marks_.clear(); }
void InstructionEncoder::markSource(const std::string& file, unsigned line, unsigned column) {
    marks_.push_back(SourceMark(code_.size(), file, line, column));
}
void InstructionEncoder::emit(unsigned char byte) { code_.push_back(byte); }
void InstructionEncoder::emit32(unsigned value) {
    for (unsigned shift = 0; shift < 32; shift += 8)
        emit(static_cast<unsigned char>((value >> shift) & 255U));
}
void InstructionEncoder::storeEaxAtRbp(int d) { emit(0x89); emit(0x85); emit32(static_cast<unsigned>(d)); }
void InstructionEncoder::loadEaxFromRbp(int d) { emit(0x8b); emit(0x85); emit32(static_cast<unsigned>(d)); }
// Verified encodings use signed disp32 from RBP. Byte stores do not
// overwrite adjacent scalar slots; MOVZX zero-extends a char load.
void InstructionEncoder::storeAlAtRbp(int d) { emit(0x88); emit(0x85); emit32(static_cast<unsigned>(d)); }
void InstructionEncoder::loadAlZeroExtendedFromRbp(int d) { emit(0x0f); emit(0xb6); emit(0x85); emit32(static_cast<unsigned>(d)); }
// IEEE-754 binary64 stack storage: MOVSD [RBP+disp32],XMM0 and inverse.
void InstructionEncoder::storeXmm0AtRbp(int d) { emit(0xf2); emit(0x0f); emit(0x11); emit(0x85); emit32(static_cast<unsigned>(d)); }
void InstructionEncoder::loadXmm0FromRbp(int d) { emit(0xf2); emit(0x0f); emit(0x10); emit(0x85); emit32(static_cast<unsigned>(d)); }
void InstructionEncoder::pushRax() { emit(0x50); }
void InstructionEncoder::popRcx() { emit(0x59); }
void InstructionEncoder::addEaxEcx() { emit(0x01); emit(0xc8); }
void InstructionEncoder::subEcxEax() { emit(0x29); emit(0xc1); emit(0x89); emit(0xc8); }
void InstructionEncoder::imulEaxEcx() { emit(0x0f); emit(0xaf); emit(0xc1); }
void InstructionEncoder::pushRbp() { emit(0x55); }
void InstructionEncoder::movRbpRsp() { emit(0x48); emit(0x89); emit(0xe5); }
void InstructionEncoder::popRbp() { emit(0x5d); }
void InstructionEncoder::movEaxImmediate(unsigned value) { emit(0xb8); emit32(value); }
void InstructionEncoder::addEaxImmediate(unsigned value) { emit(0x05); emit32(value); }
void InstructionEncoder::subEaxImmediate(unsigned value) { emit(0x2D); emit32(value); }
void InstructionEncoder::multiplyEaxImmediate(unsigned value) {
    // IMUL EAX, EAX, imm32; the low 32 bits implement Shalimar int multiplication.
    emit(0x69); emit(0xc0); emit32(value);
}
void InstructionEncoder::movEaxEdx() { emit(0x89); emit(0xd0); }
void InstructionEncoder::movEcxImmediate(unsigned value) { emit(0xb9); emit32(value); }
void InstructionEncoder::movEaxEcx() { emit(0x89); emit(0xc8); }
void InstructionEncoder::movRaxImmediate64(unsigned low, unsigned high) {
    emit(0x48); emit(0xb8); emit32(low); emit32(high);
}
void InstructionEncoder::movqXmm0Rax() { emit(0x66); emit(0x48); emit(0x0f); emit(0x6e); emit(0xc0); }
void InstructionEncoder::movqXmm1Rax() { emit(0x66); emit(0x48); emit(0x0f); emit(0x6e); emit(0xc8); }
void InstructionEncoder::movsdXmm1Xmm0() { emit(0xf2); emit(0x0f); emit(0x10); emit(0xc8); }
void InstructionEncoder::subsdXmm1Xmm0() { emit(0xf2); emit(0x0f); emit(0x5c); emit(0xc8); }
void InstructionEncoder::movsdXmm0Xmm1() { emit(0xf2); emit(0x0f); emit(0x10); emit(0xc1); }
void InstructionEncoder::addsdXmm0Xmm1() { emit(0xf2); emit(0x0f); emit(0x58); emit(0xc1); }
void InstructionEncoder::subsdXmm0Xmm1() { emit(0xf2); emit(0x0f); emit(0x5c); emit(0xc1); }
void InstructionEncoder::storeEaxAtRcx(unsigned offset) {
    emit(0x89); emit(0x81); emit32(offset);
}
void InstructionEncoder::storeEdxAtRcx(unsigned offset) {
    emit(0x89); emit(0x91); emit32(offset);
}
void InstructionEncoder::storeXmm1AtRcx(unsigned offset) {
    // MOVSD qword ptr [RCX + disp32], XMM1 (Win64 second argument slot).
    emit(0xf2); emit(0x0f); emit(0x11); emit(0x89); emit32(offset);
}
void InstructionEncoder::storeDlAtRcx(unsigned offset) {
    emit(0x88); emit(0x91); emit32(offset);
}
void InstructionEncoder::storeRaxAtRcx(unsigned offset) {
    emit(0x48); emit(0x89); emit(0x81); emit32(offset);
}
void InstructionEncoder::storeAlAtRcx(unsigned offset) {
    emit(0x88); emit(0x81); emit32(offset);
}
void InstructionEncoder::movRaxRcx() { emit(0x48); emit(0x89); emit(0xc8); }
void InstructionEncoder::ret() { emit(0xc3); }
void InstructionEncoder::allocateStack(unsigned size) { emit(0x48); emit(0x81); emit(0xec); emit32(size); }
void InstructionEncoder::releaseStack(unsigned size) { emit(0x48); emit(0x81); emit(0xc4); emit32(size); }
void InstructionEncoder::leaRcxRsp(unsigned offset) { emit(0x48); emit(0x8d); emit(0x8c); emit(0x24); emit32(offset); }
void InstructionEncoder::loadEaxFromRsp(unsigned offset) { emit(0x8b); emit(0x84); emit(0x24); emit32(offset); }
void InstructionEncoder::loadAlFromRsp(unsigned offset) {
    // MOV AL, byte ptr [RSP + disp32]; upper RAX bits are not part of char result.
    emit(0x8a); emit(0x84); emit(0x24); emit32(offset);
}
void InstructionEncoder::loadXmm0FromRsp(unsigned offset) {
    // MOVSD XMM0, qword ptr [RSP + disp32].
    emit(0xf2); emit(0x0f); emit(0x10); emit(0x84); emit(0x24); emit32(offset);
}
void InstructionEncoder::reserveCallFrame() { emit(0x48); emit(0x83); emit(0xec); emit(0x28); }
void InstructionEncoder::releaseCallFrame() { emit(0x48); emit(0x83); emit(0xc4); emit(0x28); }
unsigned InstructionEncoder::callRel32Placeholder() { emit(0xe8); const unsigned offset=static_cast<unsigned>(code_.size()); emit32(0); return offset; }
const std::vector<unsigned char>& InstructionEncoder::bytes() const { return code_; }
const std::vector<SourceMark>& InstructionEncoder::sourceMarks() const { return marks_; }
}
}
