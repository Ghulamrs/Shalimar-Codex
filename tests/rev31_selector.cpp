#include "backend/x86_64/IRSelector.h"
#include "backend/tms6747/TargetContract.h"
#include <cassert>
#include <string>
int main() {
    shalimar::IRFunction f;
    f.name = "answer";
    f.signature.outputs.push_back(shalimar::IRType("int", 0, false));
    f.blocks.push_back(shalimar::IRBasicBlock(0));
    f.blocks[0].instructions.push_back(shalimar::IRInstruction("binary +", "%t0", "40", "2", 1));
    f.blocks[0].instructions.push_back(shalimar::IRInstruction("return", "", "%t0", "", 2));
    shalimar::x86_64::InstructionEncoder e;
    shalimar::x86_64::IRSelector s;
    std::string error;
    assert(s.selectConstantReturn(f, e, error));
    assert(e.bytes().size() == 6 && e.bytes()[0] == 0xb8 && e.bytes()[1] == 42 && e.bytes()[5] == 0xc3);
    f.blocks[0].instructions[0].opcode = "call unknown";
    assert(!s.selectConstantReturn(f, e, error));
    assert(e.bytes().empty());
    assert(shalimar::tms6747::target().architecture == "tms6747");
}
