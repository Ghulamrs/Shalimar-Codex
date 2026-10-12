#include "IR.h"
#include <set>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace shalimar {
namespace {
bool isIdentifier(const std::string& s) {
    if (s.empty() || !(std::isalpha(static_cast<unsigned char>(s[0])) || s[0] == '_')) return false;
    for (size_t i = 1; i < s.size(); ++i)
        if (!(std::isalnum(static_cast<unsigned char>(s[i])) || s[i] == '_')) return false;
    return true;
}
bool isNumber(const std::string& s, bool& real) {
    if (s.empty()) return false;
    char* end = 0;
    std::strtod(s.c_str(), &end);
    if (end == s.c_str() || *end != '\0') return false;
    real = s.find_first_of(".eE") != std::string::npos;
    return true;
}
IROperandKind classifyOperand(const std::string& s) {
    if (s.empty()) return IR_OPERAND_EMPTY;
    if (s.size() > 2 && s[0] == '%' && s[1] == 't') {
        for (size_t i = 2; i < s.size(); ++i)
            if (!std::isdigit(static_cast<unsigned char>(s[i]))) return IR_OPERAND_SPECIAL;
        return IR_OPERAND_TEMPORARY;
    }
    if (s[0] == '"') return s.size() >= 2 && s[s.size()-1] == '"'
        ? IR_OPERAND_STRING_LITERAL : IR_OPERAND_SPECIAL;
    bool real = false;
    if (isNumber(s, real)) return real ? IR_OPERAND_REAL_LITERAL : IR_OPERAND_INTEGER_LITERAL;
    if (isIdentifier(s)) return IR_OPERAND_IDENTIFIER;
    if (s.find(',') != std::string::npos) return IR_OPERAND_COMPOSITE;
    return IR_OPERAND_SPECIAL;
}

std::string number(int n) { std::ostringstream out; out << n; return out.str(); }
bool fail(std::string& error, const std::string& text) { error = text; return false; }
}
IROperand::IROperand(const std::string& value)
    : kind(classifyOperand(value)), spelling(value) {}

bool IRVerifier::verify(const IRProgram& program, std::string& error) const {
    error.clear();
    std::set<std::string> imports;
    for (size_t u = 0; u < program.libraryImports.size(); ++u) {
        const IRLibraryImport& imported = program.libraryImports[u];
        if (!isIdentifier(imported.name) || !isIdentifier(imported.linkerSymbol) ||
            (imported.arity != 1 && imported.arity != 2) ||
            imported.result.name.empty() || imported.result.arrayRank != 0 ||
            imported.result.byReference ||
            imported.arguments.size() != static_cast<size_t>(imported.arity) ||
            (imported.kind != IR_LIBRARY_EXTERNAL_MATH &&
             imported.kind != IR_LIBRARY_POLYMORPHIC && imported.kind != IR_LIBRARY_INTRINSIC) ||
            (imported.kind == IR_LIBRARY_INTRINSIC && (imported.name != "len" || imported.result.name != "int")) ||
            (imported.kind == IR_LIBRARY_POLYMORPHIC && imported.result.name != "unresolved") ||
            (imported.kind == IR_LIBRARY_EXTERNAL_MATH && imported.result.name != "real"))
            return fail(error, "invalid native library import descriptor");
        if (!imports.insert(imported.name).second)
            return fail(error, "duplicate native library import: " + imported.name);
    }
    for (size_t ci = 0; ci < program.libraryCalls.size(); ++ci) {
        const IRLibraryCall& call = program.libraryCalls[ci];
        bool found = false;
        for (size_t ui = 0; ui < program.libraryImports.size(); ++ui) {
            const IRLibraryImport& imported = program.libraryImports[ui];
            if (imported.name != call.functionName) continue;
            found = true;
            if (call.argumentTypes.size() != static_cast<size_t>(imported.arity) ||
                call.resultValue.empty() || call.resultType.name.empty() ||
                call.sourceLine < 0)
                return fail(error, "invalid resolved library call: " + call.functionName);
            for (size_t ai = 0; ai < call.argumentTypes.size(); ++ai)
                if (call.argumentTypes[ai].name.empty() || call.argumentTypes[ai].arrayRank != 0)
                    return fail(error, "invalid library call argument type");
            if (imported.kind == IR_LIBRARY_EXTERNAL_MATH && call.resultType.name != "real")
                return fail(error, "invalid external math call result type");
            break;
        }
        if (!found) return fail(error, "library call without import: " + call.functionName);
    }
    std::set<std::string> names;
    for (size_t f = 0; f < program.functions.size(); ++f) {
        const IRFunction& function = program.functions[f];
        for (size_t t = 0; t < function.signature.inputs.size(); ++t) {
            const IRType& type = function.signature.inputs[t];
            if (type.name.empty() || type.arrayRank < 0)
                return fail(error, "invalid input type in " + function.name);
        }
        for (size_t t = 0; t < function.signature.outputs.size(); ++t) {
            const IRType& type = function.signature.outputs[t];
            if (type.name.empty() || type.arrayRank < 0 || type.byReference)
                return fail(error, "invalid output type in " + function.name);
        }
        if (function.debugLocation.line > 0 && function.debugLocation.file.empty())
            return fail(error, "function debug location missing file in " + function.name);
        if (function.name.empty()) return fail(error, "empty function name");
        if (!names.insert(function.name).second) return fail(error, "duplicate function: " + function.name);
        if (function.blocks.empty()) return fail(error, "function has no entry block: " + function.name);
        for (size_t b = 0; b < function.blocks.size(); ++b) {
            const IRBasicBlock& block = function.blocks[b];
            if (block.id != static_cast<int>(b)) return fail(error, "noncanonical block ID in " + function.name);
            std::set<int> edges;
            for (size_t e = 0; e < block.successors.size(); ++e) {
                int target = block.successors[e];
                if (target < 0 || static_cast<size_t>(target) >= function.blocks.size())
                    return fail(error, "invalid successor bb" + number(target) + " in " + function.name);
                if (!edges.insert(target).second) return fail(error, "duplicate successor in " + function.name);
            }
            for (size_t i = 0; i < block.instructions.size(); ++i) {
                const IRInstruction& inst = block.instructions[i];
                if (inst.opcode.empty()) return fail(error, "empty opcode in " + function.name);
                if (inst.opcode == "return" || inst.opcode == "jump" || inst.opcode == "branch") {
                    if (i + 1 != block.instructions.size())
                        return fail(error, "instruction after terminator in " + function.name);
                    const size_t expected = inst.opcode == "return" ? 0U : inst.opcode == "jump" ? 1U : 2U;
                    if (block.successors.size() != expected)
                        return fail(error, "terminator successor count mismatch in " + function.name);
                }
                if (inst.opcode.compare(0, 5, "call ") == 0) {
                    const std::string callee = inst.opcode.substr(5);
                    if (callee.empty()) return fail(error, "empty call target in " + function.name);
                    // Imported calls are legal IR, but native code emission must
                    // still reject them until a target ABI implementation exists.
                }
                if (inst.resultValue.spelling != inst.result ||
                    inst.firstValue.spelling != inst.operand1 ||
                    inst.secondValue.spelling != inst.operand2)
                    return fail(error, "operand descriptor out of sync in " + function.name);
                if (inst.resultValue.kind != classifyOperand(inst.result) ||
                    inst.firstValue.kind != classifyOperand(inst.operand1) ||
                    inst.secondValue.kind != classifyOperand(inst.operand2))
                    return fail(error, "invalid operand descriptor in " + function.name);
                if (inst.sourceLine < 0 || inst.debugLocation.line < 0 || inst.debugLocation.column < 0)
                    return fail(error, "negative source location in " + function.name);
                if (inst.debugLocation.line && inst.debugLocation.line != inst.sourceLine)
                    return fail(error, "source line metadata mismatch in " + function.name);
                if (inst.debugLocation.line && inst.debugLocation.file.empty())
                    return fail(error, "debug location missing source file in " + function.name);
            }
        }
    }
    return true;
}
} // namespace shalimar
