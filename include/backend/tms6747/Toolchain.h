#ifndef SHALIMAR_TMS6747_TOOLCHAIN_H
#define SHALIMAR_TMS6747_TOOLCHAIN_H
#include "IR.h"
#include <string>
namespace shalimar { namespace tms6747 {
class Toolchain {
public:
    bool build(const IRProgram& program, const std::string& output,
               const std::string& root, const std::string& entry,
               bool release, bool symbols, bool objectOnly, std::string& error,
               bool ownTools = false, const std::string& runtime = "") const;
};
}}
#endif
