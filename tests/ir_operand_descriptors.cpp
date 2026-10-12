#include "IR.h"
#include <cassert>
#include <string>
int main() {
    using namespace shalimar;
    assert(IROperand("%t17").kind == IR_OPERAND_TEMPORARY);
    assert(IROperand("local_1").kind == IR_OPERAND_IDENTIFIER);
    assert(IROperand("-25").kind == IR_OPERAND_INTEGER_LITERAL);
    assert(IROperand("1.25e-3").kind == IR_OPERAND_REAL_LITERAL);
    assert(IROperand("\"abc\"").kind == IR_OPERAND_STRING_LITERAL);
    assert(IROperand("a, b").kind == IR_OPERAND_COMPOSITE);
    IRProgram program;
    IRFunction function;
    function.name = "test";
    function.blocks.push_back(IRBasicBlock(0));
    function.blocks[0].instructions.push_back(IRInstruction("return", "", "1", "", 1));
    program.functions.push_back(function);
    std::string error;
    IRVerifier verifier;
    assert(verifier.verify(program, error));
    program.functions[0].blocks[0].instructions[0].firstValue.kind = IR_OPERAND_IDENTIFIER;
    assert(!verifier.verify(program, error));
    program.functions[0].blocks[0].instructions[0].firstValue = IROperand("1");
    program.functions[0].blocks[0].instructions[0].operand1 = "2";
    assert(!verifier.verify(program, error));
}
