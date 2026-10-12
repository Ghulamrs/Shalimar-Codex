#include "backend/x86_64/InstructionEncoder.h"
#include <cassert>
#include <vector>
int main() {
    shalimar::x86_64::InstructionEncoder encoder;
    encoder.markSource("example.shm", 1, 1);
    encoder.pushRbp();
    encoder.movRbpRsp();
    encoder.markSource("example.shm", 2, 5);
    encoder.movEaxImmediate(40);
    encoder.addEaxImmediate(2);
    encoder.popRbp();
    encoder.ret();
    const unsigned char expected[] = {
        0x55, 0x48, 0x89, 0xe5, 0xb8, 40, 0, 0, 0, 0x05, 2, 0, 0, 0, 0x5d, 0xc3
    };
    const std::vector<unsigned char>& bytes = encoder.bytes();
    assert(bytes.size() == sizeof(expected));
    for (std::size_t i = 0; i < sizeof(expected); ++i) assert(bytes[i] == expected[i]);
    assert(encoder.sourceMarks().size() == 2);
    assert(encoder.sourceMarks()[0].offset == 0);
    assert(encoder.sourceMarks()[1].offset == 4);
    assert(encoder.sourceMarks()[1].line == 2);
    encoder.clear();
    assert(encoder.bytes().empty() && encoder.sourceMarks().empty());
}
