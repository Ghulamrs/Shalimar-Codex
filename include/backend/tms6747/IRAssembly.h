#ifndef SHALIMAR_TMS6747_IR_ASSEMBLY_H
#define SHALIMAR_TMS6747_IR_ASSEMBLY_H
#include "IR.h"
#include <string>
namespace shalimar { namespace tms6747 {
class IRAssembly {
public:
    bool emit(const IRProgram& program, std::string& assembly, std::string& error, bool symbols = false) const;
};
}}
#endif
