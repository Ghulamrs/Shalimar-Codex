#ifndef SHALIMAR_X86_64_IR_SELECTOR_H
#define SHALIMAR_X86_64_IR_SELECTOR_H
#include "IR.h"
#include "backend/x86_64/InstructionEncoder.h"
#include <string>
#include "backend/coff/CoffWriter.h"
namespace shalimar { namespace x86_64 {
/* Deliberately restricted selection, independent of C generation. */
class IRSelector {
public:
    bool selectConstantReturn(const IRFunction& function, InstructionEncoder& encoder,
                              std::string& error) const;
    bool selectFunction(const IRFunction& function, const std::vector<IRFunction>& allFunctions, InstructionEncoder& encoder,
                        std::vector<coff::Relocation>& relocations, std::string& error,
                        std::vector<coff::DebugVariable>* debugVariables = 0) const;
};
}}
#endif
