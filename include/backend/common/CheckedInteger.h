#ifndef SHALIMAR_CHECKED_INTEGER_H
#define SHALIMAR_CHECKED_INTEGER_H
#include <climits>
#include <string>
#include "IR.h"
#include <map>
#include <cerrno>
#include <cstdlib>
#include <sstream>
namespace shalimar {
// Constant evaluation must use signed language values, never wrapping bits.
inline int signedIntegerBits(unsigned bits) {
    return bits <= static_cast<unsigned>(INT_MAX) ? static_cast<int>(bits)
        : -1 - static_cast<int>(~bits);
}
inline bool checkedIntegerBinary(const std::string& op, int left, int right, int& result) {
    long double wide;
    if (op == "binary +" || op == "+") wide = static_cast<long double>(left) + right;
    else if (op == "binary -" || op == "-") wide = static_cast<long double>(left) - right;
    else if (op == "binary *" || op == "*") wide = static_cast<long double>(left) * right;
    else return false;
    if (wide < INT_MIN || wide > INT_MAX) return false;
    result = static_cast<int>(wide);
    return true;
}
inline bool knownInteger(const std::string& text, const std::map<std::string, int>& facts, int& value) {
    std::map<std::string, int>::const_iterator found = facts.find(text);
    if (found != facts.end()) { value = found->second; return true; }
    char* end = 0; errno = 0;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (text.empty() || errno || end == text.c_str() || *end || parsed < INT_MIN || parsed > INT_MAX) return false;
    value = static_cast<int>(parsed); return true;
}
// Conservative per-block validation before machine selection. Calls invalidate
// facts; real/char variables never acquire integer facts through initialization.
inline bool validateKnownIntegerArithmetic(const IRFunction& f, std::string& error) {
    std::map<std::string, std::string> types;
    for (std::size_t p = 0; p < f.parameterNames.size() && p < f.signature.inputs.size(); ++p)
        types[f.parameterNames[p]] = f.signature.inputs[p].name;
    for (std::size_t b = 0; b < f.blocks.size(); ++b)
        for (std::size_t i = 0; i < f.blocks[b].instructions.size(); ++i) {
            const IRInstruction& in = f.blocks[b].instructions[i];
            if (in.opcode.find("declare ") == 0) types[in.result] = in.opcode.substr(8);
        }
    for (std::size_t b = 0; b < f.blocks.size(); ++b) {
        std::map<std::string, int> facts;
        for (std::size_t i = 0; i < f.blocks[b].instructions.size(); ++i) {
            const IRInstruction& in = f.blocks[b].instructions[i];
            if (in.opcode.find("call ") == 0 || in.opcode == "multi.assign") { facts.clear(); continue; }
            int left = 0, right = 0, result = 0;
            const bool arithmetic = in.opcode == "binary +" || in.opcode == "binary -" || in.opcode == "binary *";
            if (arithmetic && knownInteger(in.operand1, facts, left) && knownInteger(in.operand2, facts, right)) {
                if (!checkedIntegerBinary(in.opcode, left, right, result)) {
                    std::ostringstream detail;
                    detail << "line " << in.sourceLine << ": integer overflow in constant expression in " << f.name;
                    error = detail.str(); return false;
                }
                facts[in.result] = result;
            } else if ((in.opcode == "initialize" || in.opcode == "assign =" || in.opcode == "assign :" || in.opcode == "constant") &&
                       (types[in.result] == "int" || in.result.find("%t") == 0) && knownInteger(in.operand1, facts, left)) {
                facts[in.result] = left;
            } else if (!in.result.empty()) facts.erase(in.result);
        }
    }
    return true;
}
}
#endif
