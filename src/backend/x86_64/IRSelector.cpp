#include "backend/x86_64/IRSelector.h"
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <map>
#include <set>
#include <cstring>
#include <cmath>
#include <cfloat>
#include <sstream>
namespace shalimar { namespace x86_64 {
static bool parseInteger(const std::string& s, unsigned& value) {
    if (s.empty()) return false;
    char* end = 0;
    errno = 0;
    const long v = std::strtol(s.c_str(), &end, 10);
    if (errno || !end || *end || v < INT_MIN || v > INT_MAX) return false;
    value = static_cast<unsigned>(static_cast<int>(v));
    return true;
}
static bool resolve(const std::string& s, const std::map<std::string,unsigned>& values, unsigned& value) {
    if (parseInteger(s, value)) return true;
    const std::map<std::string,unsigned>::const_iterator it = values.find(s);
    if (it == values.end()) return false;
    value = it->second;
    return true;
}
// Evaluate one return argument through the ordinary integer SSA definitions.
// Each argument is resolved independently; cycles and duplicate definitions
// are rejected rather than silently selecting an arbitrary instruction.
static bool evaluateConstantArgument(const std::string& operand,
                                     const std::vector<IRInstruction>& body,
                                     unsigned& value, unsigned depth) {
    if (depth > 64U) return false;
    if (parseInteger(operand, value)) return true;
    const IRInstruction* definition = 0;
    for (std::size_t k = 0; k < body.size(); ++k)
        if (body[k].result == operand) {
            if (definition != 0) return false;
            definition = &body[k];
        }
    if (!definition || (definition->opcode != "binary +" &&
                        definition->opcode != "binary -" &&
                        definition->opcode != "binary *")) return false;
    unsigned left = 0, right = 0;
    if (!evaluateConstantArgument(definition->operand1, body, left, depth + 1U) ||
        !evaluateConstantArgument(definition->operand2, body, right, depth + 1U)) return false;
    value = definition->opcode == "binary +" ? left + right :
            definition->opcode == "binary -" ? left - right : left * right;
    return true;
}
// Lower a bounded chain of integer additions/subtractions rooted in the
// incoming integer parameter. Every intermediate is an SSA-style IR value.
// This is deliberately conservative: unsupported trees must fail selection.
static bool emitParameterArithmetic(const std::string& operand,
                                    const std::string& parameter,
                                    const std::vector<IRInstruction>& body,
                                    InstructionEncoder& encoder,
                                    unsigned depth) {
    if (depth > 64U) return false;
    if (operand == parameter) {
        encoder.movEaxEdx();
        return true;
    }
    const IRInstruction* expression = 0;
    for (std::size_t k = 0; k < body.size(); ++k)
        if (body[k].result == operand) {
            if (expression != 0) return false;
            expression = &body[k];
        }
    if (!expression || (expression->opcode != "binary +" &&
                        expression->opcode != "binary -" &&
                        expression->opcode != "binary *")) return false;
    unsigned immediate = 0;
    std::string runtimeOperand = expression->operand1;
    if (!parseInteger(expression->operand2, immediate)) {
        // Addition and multiplication are commutative; the constant may
        // occur on either side of a runtime SSA expression. Subtraction
        // is intentionally not reordered because its meaning differs.
        if (expression->opcode == "binary -" ||
            !parseInteger(expression->operand1, immediate)) return false;
        runtimeOperand = expression->operand2;
    }
    if (!emitParameterArithmetic(runtimeOperand, parameter, body,
                                 encoder, depth + 1U)) return false;
    if (expression->opcode == "binary +") encoder.addEaxImmediate(immediate);
    else if (expression->opcode == "binary -") encoder.subEaxImmediate(immediate);
    else encoder.multiplyEaxImmediate(immediate);
    return true;
}
bool IRSelector::selectConstantReturn(const IRFunction& f, InstructionEncoder& e, std::string& error) const {
    e.clear();
    if ((f.signature.inputs.size() > 1 ||
         (f.signature.inputs.size() == 1 &&
          (f.signature.inputs[0].name != "int" || f.signature.inputs[0].arrayRank != 0))) ||
        (f.signature.outputs.size() > 1 ||
         (f.signature.outputs.size() == 1 && (f.signature.outputs[0].name != "int" || f.signature.outputs[0].arrayRank != 0))) ||
        f.blocks.size() != 1) {
        error = "rev31 selector supports only zero-argument, single-int-return, single-block functions";
        return false;
    }
    std::map<std::string,unsigned> values;
    bool returned = false;
    unsigned result = 0;
    const IRBasicBlock& block = f.blocks[0];
    if (!block.successors.empty()) { error = "control flow is not supported by rev31 selector"; return false; }
    for (std::size_t i = 0; i < block.instructions.size(); ++i) {
        const IRInstruction& in = block.instructions[i];
        if (returned) { error = "instruction after return"; return false; }
        if (in.opcode == "binary +" || in.opcode == "binary -") {
            unsigned a = 0, b = 0;
            if (!resolve(in.operand1, values, a) || !resolve(in.operand2, values, b) || in.result.empty()) {
                error = "unresolved constant binary operands"; return false;
            }
            values[in.result] = in.opcode == "binary +" ? a + b : a - b;
        } else if (in.opcode == "return") {
            if (f.signature.outputs.empty() && in.operand1.empty()) result = 0;
            else if (!resolve(in.operand1, values, result)) { error = "nonconstant or multiple return values"; return false; }
            returned = true;
            if (!in.debugLocation.file.empty() && in.debugLocation.line > 0)
                e.markSource(in.debugLocation.file, static_cast<unsigned>(in.debugLocation.line),
                             static_cast<unsigned>(in.debugLocation.column > 0 ? in.debugLocation.column : 1));
        } else { error = "unsupported IR opcode: " + in.opcode; return false; }
    }
    if (!returned) { error = "missing return"; return false; }
    e.movEaxImmediate(result);
    e.ret();
    error.clear();
    return true;
}
}}

namespace shalimar { namespace x86_64 {
bool IRSelector::selectFunction(const IRFunction& f, const std::vector<IRFunction>& allFunctions, InstructionEncoder& e,
                                std::vector<coff::Relocation>& relocations, std::string& error,
                                std::vector<coff::DebugVariable>* debugVariables) const {
    // D4: carry source declarations through selection without asserting a
    // machine location that the selector has not established.
    if (debugVariables) {
        debugVariables->clear();
        for (std::size_t bi = 0; bi < f.blocks.size(); ++bi) {
            const std::vector<IRInstruction>& instructions = f.blocks[bi].instructions;
            for (std::size_t ii = 0; ii < instructions.size(); ++ii) {
                const IRInstruction& ins = instructions[ii];
                if (ins.opcode.compare(0, 8, "declare ") != 0) continue;
                const std::string declaredType = ins.opcode.substr(8);
                if (declaredType != "int" && declaredType != "real" && declaredType != "char") continue;
                const std::string variableName = ins.result;
                if (variableName.empty()) continue;
                bool duplicate = false;
                for (std::size_t vi = 0; vi < debugVariables->size(); ++vi)
                    if ((*debugVariables)[vi].name == variableName) duplicate = true;
                if (duplicate) continue;
                coff::DebugVariable variable;
                variable.name = variableName;
                variable.typeName = declaredType;
                variable.location = coff::DebugVariable::Unavailable;
                debugVariables->push_back(variable);
            }
        }
    }
    relocations.clear();
    // D4.10: restricted native real locals. Every real occupies a genuine
    // binary64 RBP-relative slot; Windows x64 returns real through XMM0.
    if (f.signature.inputs.empty() && f.signature.outputs.size() == 1 &&
        f.signature.outputs[0].name == "real" && f.signature.outputs[0].arrayRank == 0 &&
        f.blocks.size() == 1 && f.blocks[0].successors.empty()) {
        const std::vector<IRInstruction>& body = f.blocks[0].instructions;
        std::map<std::string,int> slots;
        bool shape = !body.empty() && body.back().opcode == "return";
        for (std::size_t i = 0; i < body.size() && shape; ++i) {
            const IRInstruction& in = body[i];
            if (in.opcode == "declare real") {
                if (in.result.empty() || slots.count(in.result)) shape = false;
                else slots[in.result] = -static_cast<int>((slots.size() + 1U) * 8U);
            } else if (in.opcode == "initialize" || in.opcode == "assign =") {
                if (!slots.count(in.result)) shape = false;
            } else if (in.opcode == "binary +" || in.opcode == "binary -") {
                if (in.result.empty() || slots.count(in.result)) shape = false;
                else slots[in.result] = -static_cast<int>((slots.size() + 1U) * 8U);
            } else if (in.opcode != "return" || i + 1U != body.size()) shape = false;
        }
        if (shape && !slots.empty() && slots.size() <= 256U) {
            InstructionEncoder candidate;
            candidate.pushRbp(); candidate.movRbpRsp();
            const unsigned frame = static_cast<unsigned>((slots.size() * 8U + 15U) & ~15U);
            candidate.allocateStack(frame);
            std::set<std::string> initialized;
            std::map<std::string,unsigned> firstStore;
            for (std::size_t i = 0; i < body.size() && shape; ++i) {
                const IRInstruction& in = body[i];
                if (!in.debugLocation.file.empty() && in.debugLocation.line > 0)
                    candidate.markSource(in.debugLocation.file, static_cast<unsigned>(in.debugLocation.line), 1U);
                if (in.opcode == "declare real") continue;
                // Load a real operand into XMM0, either from a verified slot
                // or by copying the exact IEEE-754 literal bits through RAX.
                const std::string operands[2] = { in.operand1, in.operand2 };
                const unsigned count = (in.opcode == "binary +" || in.opcode == "binary -") ? 2U : 1U;
                for (unsigned j = 0; j < count && shape; ++j) {
                    const std::string& op = operands[j];
                    if (slots.count(op) && initialized.count(op))
                        candidate.loadXmm0FromRbp(slots[op]);
                    else {
                        char* end = 0;
                        errno = 0;
                        const double value = std::strtod(op.c_str(), &end);
                        if (op.empty() || !end || *end || errno == ERANGE ||
                            value != value || value > DBL_MAX || value < -DBL_MAX) {
                            shape = false; break;
                        }
                        unsigned words[2] = {0U, 0U};
                        if (sizeof(value) != 8U || sizeof(words) != 8U) { shape = false; break; }
                        std::memcpy(words, &value, 8U);
                        candidate.movRaxImmediate64(words[0], words[1]);
                        candidate.movqXmm0Rax();
                    }
                    if (j == 0U && count == 2U) candidate.movsdXmm1Xmm0();
                }
                if (!shape) break;
                if (count == 2U) {
                    // XMM1 is the left operand and XMM0 the right operand.
                    // Addition commutes; subtraction must preserve order.
                    if (in.opcode == "binary +") candidate.addsdXmm0Xmm1();
                    else { candidate.subsdXmm1Xmm0(); candidate.movsdXmm0Xmm1(); }
                }
                if (in.opcode != "return") {
                    candidate.storeXmm0AtRbp(slots[in.result]);
                    initialized.insert(in.result);
                    if (!firstStore.count(in.result))
                        firstStore[in.result] = static_cast<unsigned>(candidate.bytes().size());
                }
            }
            if (shape) {
                const unsigned end = static_cast<unsigned>(candidate.bytes().size());
                candidate.releaseStack(frame); candidate.popRbp(); candidate.ret();
                e = candidate;
                if (debugVariables) for (std::size_t k = 0; k < debugVariables->size(); ++k) {
                    coff::DebugVariable& v = (*debugVariables)[k];
                    if (v.typeName == "real" && slots.count(v.name) && initialized.count(v.name)) {
                        v.location = coff::DebugVariable::FrameRelative;
                        v.frameOffset = slots[v.name];
                        if (firstStore.count(v.name) && firstStore[v.name] < end) {
                            v.liveStart = firstStore[v.name]; v.liveEnd = end; v.hasLiveRange = true;
                        }
                    }
                }
                error.clear(); return true;
            }
        }
    }
    // D4.10: restricted native char-local lowering with genuine byte slots.
    // A char is stored as one byte; loads zero-extend into EAX, consistent
    // with the scalar unsigned-byte native representation. Other forms fail
    // through the existing conservative selector.
    if (f.signature.inputs.empty() && f.signature.outputs.size() == 1 &&
        f.signature.outputs[0].name == "char" && f.signature.outputs[0].arrayRank == 0 &&
        f.blocks.size() == 1 && f.blocks[0].successors.empty()) {
        const std::vector<IRInstruction>& body = f.blocks[0].instructions;
        std::map<std::string,int> slots;
        bool shape = !body.empty() && body.back().opcode == "return";
        for (std::size_t i=0; i<body.size() && shape; ++i) {
            const IRInstruction& in=body[i];
            if (in.opcode == "declare char") {
                if (in.result.empty() || slots.count(in.result)) shape=false;
                else slots[in.result]=-static_cast<int>(slots.size()+1U);
            } else if (in.opcode == "initialize" || in.opcode == "assign =") {
                if (!slots.count(in.result)) shape=false;
            } else if (in.opcode != "return" || i+1U != body.size()) shape=false;
        }
        if (shape && !slots.empty() && slots.size() <= 256U) {
            InstructionEncoder candidate;
            candidate.pushRbp(); candidate.movRbpRsp();
            const unsigned frame=static_cast<unsigned>((slots.size()+15U)&~15U);
            candidate.allocateStack(frame);
            std::set<std::string> initialized;
            std::map<std::string,unsigned> firstStore;
            for (std::size_t i=0; i<body.size() && shape; ++i) {
                const IRInstruction& in=body[i];
                if (!in.debugLocation.file.empty() && in.debugLocation.line > 0)
                    candidate.markSource(in.debugLocation.file, static_cast<unsigned>(in.debugLocation.line), 1U);
                if (in.opcode == "declare char") continue;
                unsigned literal=0;
                if (parseInteger(in.operand1,literal)) {
                    if (literal > 255U) { shape=false; break; }
                    candidate.movEaxImmediate(literal);
                } else if (slots.count(in.operand1) && initialized.count(in.operand1))
                    candidate.loadAlZeroExtendedFromRbp(slots[in.operand1]);
                else { shape=false; break; }
                if (in.opcode != "return") {
                    candidate.storeAlAtRbp(slots[in.result]);
                    initialized.insert(in.result);
                    if (!firstStore.count(in.result)) firstStore[in.result]=static_cast<unsigned>(candidate.bytes().size());
                }
            }
            if (shape) {
                const unsigned end=static_cast<unsigned>(candidate.bytes().size());
                candidate.releaseStack(frame); candidate.popRbp(); candidate.ret();
                e=candidate;
                if (debugVariables) for (std::size_t k=0; k<debugVariables->size(); ++k) {
                    coff::DebugVariable& v=(*debugVariables)[k];
                    if (v.typeName=="char" && slots.count(v.name) && initialized.count(v.name)) {
                        v.location=coff::DebugVariable::FrameRelative;
                        v.frameOffset=slots[v.name];
                        if (firstStore.count(v.name) && firstStore[v.name]<end) {
                            v.liveStart=firstStore[v.name]; v.liveEnd=end; v.hasLiveRange=true;
                        }
                    }
                }
                error.clear(); return true;
            }
        }
    }
    // D4.3: genuine RBP-relative slots for a bounded single-block int subset.
    // All definitions are evaluated at runtime; no constant substitution for locals.
    if (f.signature.inputs.empty() && f.signature.outputs.size() == 1 &&
        f.signature.outputs[0].name == "int" && f.signature.outputs[0].arrayRank == 0 &&
        f.blocks.size() == 1 && f.blocks[0].successors.empty()) {
        const std::vector<IRInstruction>& body = f.blocks[0].instructions;
        std::map<std::string,int> slots;
        bool shape = !body.empty() && body.back().opcode == "return";
        for (std::size_t i = 0; i < body.size() && shape; ++i) {
            const IRInstruction& in = body[i];
            if (in.opcode == "declare int") {
                if (in.result.empty() || slots.count(in.result)) { shape = false; break; }
                slots[in.result] = -static_cast<int>(slots.size() * 4U + 4U);
            } else if (in.opcode == "initialize" || in.opcode == "assign =") {
                if (!slots.count(in.result)) shape = false;
            } else if (in.opcode == "binary +" || in.opcode == "binary -" || in.opcode == "binary *") {
                if (in.result.empty() || slots.count(in.result)) shape = false;
                else slots[in.result] = -static_cast<int>(slots.size() * 4U + 4U);
            } else if (in.opcode != "return" || i + 1U != body.size()) shape = false;
        }
        if (shape && !slots.empty() && slots.size() <= 256U) {
            InstructionEncoder candidate;
            candidate.pushRbp(); candidate.movRbpRsp();
            const unsigned frame = static_cast<unsigned>((slots.size() * 4U + 15U) & ~15U);
            candidate.allocateStack(frame);
            // A stack slot is not a live debugger value until its first store.
            // Reject any read before a dominating initialization in this
            // single-block selector; never expose uninitialized stack bytes.
            std::set<std::string> initializedSlots;
            std::map<std::string,unsigned> firstStoreEnd;
            for (std::size_t i = 0; i < body.size() && shape; ++i) {
                const IRInstruction& in = body[i];
                if (!in.debugLocation.file.empty() && in.debugLocation.line > 0)
                    candidate.markSource(in.debugLocation.file, static_cast<unsigned>(in.debugLocation.line), 1U);
                if (in.opcode == "declare int") continue;
                if (in.opcode == "initialize" || in.opcode == "assign =" || in.opcode == "return") {
                    const std::string operand = in.opcode == "return" ? in.operand1 : in.operand1;
                    unsigned literal = 0;
                    if (parseInteger(operand, literal)) candidate.movEaxImmediate(literal);
                    else if (slots.count(operand) && initializedSlots.count(operand)) candidate.loadEaxFromRbp(slots[operand]);
                    else { shape = false; break; }
                    if (in.opcode != "return") {
                        candidate.storeEaxAtRbp(slots[in.result]);
                        initializedSlots.insert(in.result);
                        if (!firstStoreEnd.count(in.result)) firstStoreEnd[in.result] = static_cast<unsigned>(candidate.bytes().size());
                    }
                } else {
                    unsigned literal = 0;
                    if (parseInteger(in.operand1, literal)) candidate.movEaxImmediate(literal);
                    else if (slots.count(in.operand1) && initializedSlots.count(in.operand1)) candidate.loadEaxFromRbp(slots[in.operand1]);
                    else { shape = false; break; }
                    candidate.pushRax();
                    if (parseInteger(in.operand2, literal)) candidate.movEaxImmediate(literal);
                    else if (slots.count(in.operand2) && initializedSlots.count(in.operand2)) candidate.loadEaxFromRbp(slots[in.operand2]);
                    else { shape = false; break; }
                    candidate.popRcx();
                    if (in.opcode == "binary +") candidate.addEaxEcx();
                    else if (in.opcode == "binary -") candidate.subEcxEax();
                    else candidate.imulEaxEcx();
                    candidate.storeEaxAtRbp(slots[in.result]);
                    initializedSlots.insert(in.result);
                    if (!firstStoreEnd.count(in.result)) firstStoreEnd[in.result] = static_cast<unsigned>(candidate.bytes().size());
                }
            }
            if (shape) {
                const unsigned epilogueOffset = static_cast<unsigned>(candidate.bytes().size());
                candidate.releaseStack(frame); candidate.popRbp(); candidate.ret();
                e = candidate;
                if (debugVariables) {
                    for (std::size_t k = 0; k < debugVariables->size(); ++k) {
                        coff::DebugVariable& variable = (*debugVariables)[k];
                        if (variable.typeName == "int" && slots.count(variable.name) &&
                            initializedSlots.count(variable.name)) {
                            variable.location = coff::DebugVariable::FrameRelative;
                            variable.frameOffset = slots[variable.name];
                            if (firstStoreEnd.count(variable.name) && firstStoreEnd[variable.name] < epilogueOffset) {
                                variable.liveStart = firstStoreEnd[variable.name];
                                variable.liveEnd = epilogueOffset;
                                variable.hasLiveRange = true;
                            }
                        }
                    }
                }
                error.clear(); return true;
            }
        }
    }
    if (f.signature.inputs.empty() && f.signature.outputs.empty() &&
        f.blocks.size() == 1 && f.blocks[0].successors.empty() &&
        f.blocks[0].instructions.empty()) {
        e.clear(); e.ret(); error.clear(); return true;
    }
    // rev51: zero-argument multi-result call with caller-owned aligned storage.
    // The return may use a named integer result; other result fields stay in the buffer.
    if (f.signature.inputs.empty() && f.signature.outputs.size() <= 1 &&
        f.blocks.size() == 1 && f.blocks[0].successors.empty()) {
        const std::vector<IRInstruction>& ins = f.blocks[0].instructions;
        std::size_t callIndex = ins.size();
        for (std::size_t k = 0; k < ins.size(); ++k)
            if (ins[k].opcode.compare(0, 5, "call ") == 0) {
                if (callIndex != ins.size()) { callIndex = ins.size(); break; }
                callIndex = k;
            }
        if (callIndex < ins.size() && callIndex + 2 < ins.size() &&
            ins[callIndex].operand1.empty() && ins[callIndex + 1].opcode == "multi.assign" &&
            ins[callIndex + 1].operand1 == ins[callIndex].result &&
            ins.back().opcode == "return" && ins.back().result.empty()) {
            const std::string callee = ins[callIndex].opcode.substr(5);
            const IRFunction* target = 0;
            for (std::size_t k = 0; k < allFunctions.size(); ++k)
                if (allFunctions[k].name == callee && target == 0) target = &allFunctions[k];
            if (target && target != &f && target->signature.inputs.empty() &&
                target->signature.outputs.size() > 1) {
                const IRReturnLayout layout = IRReturnLayout::fromOutputs(target->signature.outputs);
                std::vector<std::string> names;
                std::istringstream ns(ins[callIndex + 1].result);
                std::string name;
                while (std::getline(ns, name, ',')) {
                    const std::string::size_type a = name.find_first_not_of(" \t");
                    if (a == std::string::npos) break;
                    const std::string::size_type b = name.find_last_not_of(" \t");
                    names.push_back(name.substr(a, b-a+1));
                }
                // Keep the return buffer inside a 16-byte-aligned frame. The first
                // 32 bytes are Win64 shadow space; locals start at RSP+32.
                const unsigned storageSize = static_cast<unsigned>((layout.size + 15U) & ~15U);
                const unsigned frameSize = 32U + storageSize;
                bool safe = layout.valid && layout.size > 0 && layout.size <= 4096U &&
                    names.size() == layout.fields.size() && frameSize <= 0x7fffffffU;
                for (std::size_t k = 0; k < ins.size() && safe; ++k) {
                    if (k == callIndex || k == callIndex + 1 || k == ins.size()-1) continue;
                    if (ins[k].opcode.compare(0, 8, "declare ") != 0) safe = false;
                }
                std::size_t resultIndex = names.size();
                if (safe && f.signature.outputs.empty()) safe = ins.back().operand1.empty();
                else if (safe && f.signature.outputs.size() == 1 &&
                         (f.signature.outputs[0].name == "int" || f.signature.outputs[0].name == "real" || f.signature.outputs[0].name == "char")) {
                    for (std::size_t k = 0; k < names.size(); ++k)
                        if (names[k] == ins.back().operand1 && layout.fields[k].type.name == f.signature.outputs[0].name)
                            resultIndex = k;
                    safe = resultIndex < names.size();
                } else safe = false;
                if (safe) {
                    e.clear();
                    // On entry RSP is 8 mod 16. Subtract an 8-mod-16 frame.
                    e.allocateStack(frameSize + 8U);
                    e.leaRcxRsp(32U);
                    coff::Relocation r;
                    r.offset = e.callRel32Placeholder();
                    r.symbol = callee;
                    relocations.push_back(r);
                    if (resultIndex < names.size()) {
                        if (f.signature.outputs[0].name == "real")
                            e.loadXmm0FromRsp(32U + layout.fields[resultIndex].offset);
                        else if (f.signature.outputs[0].name == "char")
                            e.loadAlFromRsp(32U + layout.fields[resultIndex].offset);
                        else
                            e.loadEaxFromRsp(32U + layout.fields[resultIndex].offset);
                    }
                    e.releaseStack(frameSize + 8U);
                    e.ret();
                    error.clear();
                    return true;
                }
            }
        }
    }
    // rev51: Win64 caller-owned result structure for literal multi-return functions.
    // Hidden first argument RCX points to the storage; return the same pointer in RAX.
    if ((f.signature.inputs.empty() ||
         (f.signature.inputs.size() == 1 && f.parameterNames.size() == 1 &&
          (f.signature.inputs[0].name == "int" || f.signature.inputs[0].name == "real") && f.signature.inputs[0].arrayRank == 0 &&
          !f.signature.inputs[0].byReference)) && f.signature.outputs.size() > 1 &&
        f.blocks.size() == 1 && f.blocks[0].successors.empty() &&
        !f.blocks[0].instructions.empty() &&
        f.blocks[0].instructions.back().opcode == "return") {
        const IRReturnLayout layout = IRReturnLayout::fromOutputs(f.signature.outputs);
        const IRInstruction& in = f.blocks[0].instructions.back();
        // Every pre-return instruction must be a supported, side-effect-free
        // expression. Resolve each result independently so one return can
        // contain several different arithmetic expressions.
        const std::vector<IRInstruction>& body = f.blocks[0].instructions;
        bool expressionShape = true;
        for (std::size_t k = 0; k + 1 < body.size(); ++k)
            if (body[k].opcode != "binary +" && body[k].opcode != "binary -" &&
                body[k].opcode != "binary *" && body[k].opcode != "call char") expressionShape = false;
        std::vector<std::string> parts;
        std::istringstream stream(in.operand1);
        std::string part;
        while (std::getline(stream, part, ',')) {
            const std::string::size_type first = part.find_first_not_of(" \t");
            if (first == std::string::npos) break;
            const std::string::size_type last = part.find_last_not_of(" \t");
            parts.push_back(part.substr(first, last - first + 1));
        }
        if (layout.valid && parts.size() == layout.fields.size()) {
            InstructionEncoder candidate;
            bool supported = expressionShape;
            if (!in.debugLocation.file.empty() && in.debugLocation.line > 0)
                candidate.markSource(in.debugLocation.file, static_cast<unsigned>(in.debugLocation.line), 1);
            // Use the same bounded SSA evaluator for each return argument.
            // Nonconstant operands are handled by the runtime path below.
            for (std::size_t i = 0; i < parts.size(); ++i) {
                const IRReturnField& field = layout.fields[i];
                if (field.type.name == "int" || field.type.name == "char") {
                    unsigned value = 0;
                    if (!f.parameterNames.empty() && parts[i] == f.parameterNames[0]) {
                        // Hidden structure pointer occupies RCX; first integer argument is EDX.
                        if (field.type.name == "char") { supported = false; break; }
                        candidate.storeEdxAtRcx(field.offset);
                        continue;
                    }
                    const IRInstruction* expression = 0;
                    for (std::size_t k = 0; k + 1 < body.size(); ++k)
                        if (body[k].result == parts[i]) expression = &body[k];
                    if (expression && field.type.name == "int" &&
                        !f.parameterNames.empty() && f.signature.inputs[0].name == "int") {
                        if (!emitParameterArithmetic(parts[i], f.parameterNames[0],
                                                     body, candidate, 0U)) {
                            supported = false; break;
                        }
                        candidate.storeEaxAtRcx(field.offset);
                        continue;
                    }
                    if (field.type.name == "char" && parts[i] == "char(0)") {
                        // Canonical typed zero synthesized by IRBuilder for omitted outputs.
                        value = 0U;
                    } else if (expression && field.type.name == "char" && expression->opcode == "call char") {
                        if (!parseInteger(expression->operand1, value)) { supported = false; break; }
                    } else if (!evaluateConstantArgument(parts[i], body, value, 0U)) { supported = false; break; }
                    if (field.type.name == "char" && value > 255U) { supported = false; break; }
                    candidate.movEaxImmediate(value);
                    if (field.type.name == "char") candidate.storeAlAtRcx(field.offset);
                    else candidate.storeEaxAtRcx(field.offset);
                } else if (field.type.name == "real") {
                    if (!f.parameterNames.empty() && parts[i] == f.parameterNames[0] &&
                        f.signature.inputs[0].name == "real") {
                        // Hidden return pointer in RCX; first real argument in XMM1.
                        candidate.storeXmm1AtRcx(field.offset);
                        continue;
                    }
                    char* end = 0;
                    errno = 0;
                    const double value = std::strtod(parts[i].c_str(), &end);
                    if (errno || !end || end == parts[i].c_str() || *end ||
                        value != value || value > DBL_MAX || value < -DBL_MAX ||
                        sizeof(double) != 8 || sizeof(unsigned) != 4) {
                        supported = false; break;
                    }
                    unsigned char bits[8];
                    std::memcpy(bits, &value, 8);
                    unsigned low = 0, high = 0;
                    for (unsigned j = 0; j < 4; ++j) {
                        low |= static_cast<unsigned>(bits[j]) << (j * 8);
                        high |= static_cast<unsigned>(bits[j + 4]) << (j * 8);
                    }
                    candidate.movRaxImmediate64(low, high);
                    candidate.storeRaxAtRcx(field.offset);
                } else { supported = false; break; }
            }
            if (supported) {
                candidate.movRaxRcx();
                candidate.ret();
                e = candidate;
                error.clear();
                return true;
            }
        }
    }
    // rev49 continuation: a real identity function receives and returns XMM0.
    if (f.signature.inputs.size() == 1 && f.parameterNames.size() == 1 &&
        f.signature.inputs[0].name == "real" && f.signature.inputs[0].arrayRank == 0 &&
        !f.signature.inputs[0].byReference && f.signature.outputs.size() == 1 &&
        f.signature.outputs[0].name == "real" && f.signature.outputs[0].arrayRank == 0 &&
        f.blocks.size() == 1 && f.blocks[0].successors.empty() &&
        f.blocks[0].instructions.size() == 1 &&
        f.blocks[0].instructions[0].opcode == "return" &&
        f.blocks[0].instructions[0].operand1 == f.parameterNames[0]) {
        e.clear();
        const IRInstruction& in = f.blocks[0].instructions[0];
        if (!in.debugLocation.file.empty() && in.debugLocation.line > 0)
            e.markSource(in.debugLocation.file, static_cast<unsigned>(in.debugLocation.line), 1);
        e.ret(); // XMM0 already holds the parameter and return value.
        error.clear();
        return true;
    }
    // rev49 continuation: add a finite binary64 literal to a real parameter.
    // Windows x64 supplies the parameter in XMM0; preserve it while loading XMM1.
    if (f.signature.inputs.size() == 1 && f.parameterNames.size() == 1 &&
        f.signature.inputs[0].name == "real" && f.signature.inputs[0].arrayRank == 0 &&
        !f.signature.inputs[0].byReference && f.signature.outputs.size() == 1 &&
        f.signature.outputs[0].name == "real" && f.signature.outputs[0].arrayRank == 0 &&
        f.blocks.size() == 1 && f.blocks[0].successors.empty() &&
        f.blocks[0].instructions.size() == 2 &&
        f.blocks[0].instructions[0].opcode == "binary +" &&
        f.blocks[0].instructions[1].opcode == "return" &&
        f.blocks[0].instructions[1].operand1 == f.blocks[0].instructions[0].result) {
        const IRInstruction& op = f.blocks[0].instructions[0];
        std::string literal;
        if (op.operand1 == f.parameterNames[0]) literal = op.operand2;
        else if (op.operand2 == f.parameterNames[0]) literal = op.operand1;
        if (!literal.empty()) {
            char* end = 0;
            errno = 0;
            const double value = std::strtod(literal.c_str(), &end);
            if (!errno && end && end != literal.c_str() && *end == 0 &&
                value == value && value <= DBL_MAX && value >= -DBL_MAX &&
                sizeof(double) == 8 && sizeof(unsigned) == 4) {
                unsigned char bits[8];
                std::memcpy(bits, &value, 8);
                unsigned low = 0, high = 0;
                for (unsigned i = 0; i < 4; ++i) {
                    low |= static_cast<unsigned>(bits[i]) << (i * 8);
                    high |= static_cast<unsigned>(bits[i + 4]) << (i * 8);
                }
                e.clear();
                if (!op.debugLocation.file.empty() && op.debugLocation.line > 0)
                    e.markSource(op.debugLocation.file, static_cast<unsigned>(op.debugLocation.line), 1);
                e.movRaxImmediate64(low, high);
                e.movqXmm1Rax();
                e.addsdXmm0Xmm1();
                e.ret();
                error.clear();
                return true;
            }
        }
    }
    // rev49: subtract a finite literal from the first real parameter.
    if (f.signature.inputs.size() == 1 && f.parameterNames.size() == 1 &&
        f.signature.inputs[0].name == "real" && f.signature.inputs[0].arrayRank == 0 &&
        !f.signature.inputs[0].byReference && f.signature.outputs.size() == 1 &&
        f.signature.outputs[0].name == "real" && f.signature.outputs[0].arrayRank == 0 &&
        f.blocks.size() == 1 && f.blocks[0].successors.empty() &&
        f.blocks[0].instructions.size() == 2 &&
        f.blocks[0].instructions[0].opcode == "binary -" &&
        f.blocks[0].instructions[1].opcode == "return" &&
        f.blocks[0].instructions[1].operand1 == f.blocks[0].instructions[0].result) {
        const IRInstruction& op = f.blocks[0].instructions[0];
        if (op.operand1 == f.parameterNames[0] && !op.operand2.empty()) {
            char* end = 0;
            errno = 0;
            const double value = std::strtod(op.operand2.c_str(), &end);
            if (!errno && end && end != op.operand2.c_str() && *end == 0 &&
                value == value && value <= DBL_MAX && value >= -DBL_MAX &&
                sizeof(double) == 8 && sizeof(unsigned) == 4) {
                unsigned char bits[8];
                std::memcpy(bits, &value, 8);
                unsigned low = 0, high = 0;
                for (unsigned i = 0; i < 4; ++i) {
                    low |= static_cast<unsigned>(bits[i]) << (i * 8);
                    high |= static_cast<unsigned>(bits[i + 4]) << (i * 8);
                }
                e.clear();
                if (!op.debugLocation.file.empty() && op.debugLocation.line > 0)
                    e.markSource(op.debugLocation.file, static_cast<unsigned>(op.debugLocation.line), 1);
                e.movRaxImmediate64(low, high);
                e.movqXmm1Rax();
                e.subsdXmm0Xmm1();
                e.ret();
                error.clear();
                return true;
            }
        }
    }
    // Fold a single real-literal addition at selection time; preserve binary64 rounding.
    if (f.signature.inputs.empty() && f.signature.outputs.size() == 1 &&
        f.signature.outputs[0].name == "real" && f.signature.outputs[0].arrayRank == 0 &&
        f.blocks.size() == 1 && f.blocks[0].successors.empty() &&
        f.blocks[0].instructions.size() == 2 &&
        (f.blocks[0].instructions[0].opcode == "binary +" ||
          f.blocks[0].instructions[0].opcode == "binary -") &&
        f.blocks[0].instructions[1].opcode == "return" &&
        f.blocks[0].instructions[1].operand1 == f.blocks[0].instructions[0].result) {
        const IRInstruction& op = f.blocks[0].instructions[0];
        char* end1 = 0;
        char* end2 = 0;
        errno = 0;
        const double lhs = std::strtod(op.operand1.c_str(), &end1);
        const double rhs = std::strtod(op.operand2.c_str(), &end2);
        if (!op.operand1.empty() && !op.operand2.empty() &&
            end1 && *end1 == 0 && end2 && *end2 == 0 && errno == 0 &&
            lhs == lhs && rhs == rhs && lhs <= DBL_MAX && lhs >= -DBL_MAX &&
            rhs <= DBL_MAX && rhs >= -DBL_MAX && sizeof(double) == 8 && sizeof(unsigned) == 4) {
            const double result = lhs + rhs;
            if (result == result && result <= DBL_MAX && result >= -DBL_MAX) {
                unsigned char bits[8];
                std::memcpy(bits, &result, 8);
                unsigned low = 0, high = 0;
                for (unsigned i = 0; i < 4; ++i) {
                    low |= static_cast<unsigned>(bits[i]) << (i * 8);
                    high |= static_cast<unsigned>(bits[i + 4]) << (i * 8);
                }
                e.clear();
                if (!op.debugLocation.file.empty() && op.debugLocation.line > 0)
                    e.markSource(op.debugLocation.file, static_cast<unsigned>(op.debugLocation.line), 1);
                e.movRaxImmediate64(low, high);
                e.movqXmm0Rax();
                e.ret();
                error.clear();
                return true;
            }
        }
    }
    // rev49 checkpoint: a real literal result uses the Windows x64 XMM0 ABI.
    // This deliberately does not claim general real-expression support.
    if (f.signature.inputs.empty() && f.signature.outputs.size() == 1 &&
        f.signature.outputs[0].name == "real" && f.signature.outputs[0].arrayRank == 0 &&
        f.blocks.size() == 1 && f.blocks[0].successors.empty() &&
        f.blocks[0].instructions.size() == 1 &&
        f.blocks[0].instructions[0].opcode == "return") {
        const IRInstruction& instruction = f.blocks[0].instructions[0];
        const std::string& literal = instruction.operand1;
        char* end = 0;
        errno = 0;
        const double value = std::strtod(literal.c_str(), &end);
        if (!literal.empty() && end && end != literal.c_str() && !*end &&
            errno == 0 && value == value && value <= DBL_MAX && value >= -DBL_MAX &&
            sizeof(double) == 8 && sizeof(unsigned) == 4) {
            unsigned char bits[8];
            std::memcpy(bits, &value, 8);
            unsigned low = 0, high = 0;
            for (unsigned i = 0; i < 4; ++i) {
                low |= static_cast<unsigned>(bits[i]) << (i * 8);
                high |= static_cast<unsigned>(bits[i + 4]) << (i * 8);
            }
            e.clear();
            if (!instruction.debugLocation.file.empty() && instruction.debugLocation.line > 0)
                e.markSource(instruction.debugLocation.file,
                             static_cast<unsigned>(instruction.debugLocation.line),
                             static_cast<unsigned>(instruction.debugLocation.column > 0 ? instruction.debugLocation.column : 1));
            e.movRaxImmediate64(low, high);
            e.movqXmm0Rax();
            e.ret();
            error.clear();
            return true;
        }
    }
    // rev45: direct external sqrt(double) call with an IEEE-754 literal.
    // The Win64 ABI passes its first floating argument and result in XMM0.
    // Only a known imported signature may authorize this external symbol.
    if (f.signature.inputs.empty() && f.signature.outputs.size() == 1 &&
        f.signature.outputs[0].name == "real" && f.signature.outputs[0].arrayRank == 0 &&
        f.blocks.size() == 1 && f.blocks[0].successors.empty()) {
        const std::vector<IRInstruction>& ins = f.blocks[0].instructions;
        if (ins.size() == 2 && ins[0].opcode == "call sqrt" &&
            ins[1].opcode == "return" && ins[1].operand1 == ins[0].result &&
            !ins[0].result.empty() && ins[0].operand1.find(',') == std::string::npos) {
            char* end = 0;
            errno = 0;
            const double value = std::strtod(ins[0].operand1.c_str(), &end);
            if (errno || end == ins[0].operand1.c_str() || !end || *end ||
                value != value || value > DBL_MAX || value < -DBL_MAX) {
                error = "rev45 sqrt requires a finite real literal argument";
                return false;
            }
            unsigned words[2] = {0, 0};
            if (sizeof(double) != 8 || sizeof(unsigned) != 4) {
                error = "rev45 requires 32-bit unsigned and IEEE-754 binary64";
                return false;
            }
            unsigned char bits[8];
            std::memcpy(bits, &value, 8);
            for (unsigned i = 0; i < 4; ++i) {
                words[0] |= static_cast<unsigned>(bits[i]) << (i * 8);
                words[1] |= static_cast<unsigned>(bits[i + 4]) << (i * 8);
            }
            e.clear();
            e.movRaxImmediate64(words[0], words[1]);
            e.movqXmm0Rax();
            e.reserveCallFrame();
            if (!ins[0].debugLocation.file.empty() && ins[0].debugLocation.line > 0)
                e.markSource(ins[0].debugLocation.file,
                             static_cast<unsigned>(ins[0].debugLocation.line),
                             static_cast<unsigned>(ins[0].debugLocation.column > 0 ? ins[0].debugLocation.column : 1));
            coff::Relocation r;
            r.offset = e.callRel32Placeholder();
            r.symbol = "sqrt";
            relocations.push_back(r);
            e.releaseCallFrame();
            e.ret();
            error.clear();
            return true;
        }
    }
    // Win64: the first integer parameter arrives in ECX; the int result is EAX.
    // Parameter identity is taken from the IR signature, never guessed from text.
    if (f.signature.inputs.size() == 1 && f.parameterNames.size() == 1 &&
        f.signature.inputs[0].name == "int" && f.signature.inputs[0].arrayRank == 0 &&
        !f.signature.inputs[0].byReference && f.signature.outputs.size() == 1 &&
        f.signature.outputs[0].name == "int" && f.signature.outputs[0].arrayRank == 0 &&
        f.blocks.size() == 1 && f.blocks[0].successors.empty() &&
        f.blocks[0].instructions.size() == 1 &&
        f.blocks[0].instructions[0].opcode == "return" &&
        f.blocks[0].instructions[0].operand1 == f.parameterNames[0]) {
        e.clear();
        const IRInstruction& instruction = f.blocks[0].instructions[0];
        if (!instruction.debugLocation.file.empty() && instruction.debugLocation.line > 0)
            e.markSource(instruction.debugLocation.file,
                         static_cast<unsigned>(instruction.debugLocation.line), 1);
        e.movEaxEcx();
        e.ret();
        error.clear();
        return true;
    }
    // Preserve the already verified constant-return subset.
    if (selectConstantReturn(f, e, error)) return true;
    e.clear();
    if (!f.signature.inputs.empty() || f.signature.outputs.size()!=1 ||
        f.signature.outputs[0].name!="int" || f.signature.outputs[0].arrayRank!=0 ||
        f.blocks.size()!=1 || !f.blocks[0].successors.empty()) {
        error="rev36 supports only zero-argument int functions with one basic block"; return false;
    }
    const std::vector<IRInstruction>& instructions=f.blocks[0].instructions;
    if (instructions.size()!=2 || instructions[0].opcode.compare(0,5,"call ")!=0 ||
        instructions[0].opcode.size()<=5 ||
        instructions[0].result.empty() || instructions[1].opcode!="return" ||
        instructions[1].operand1!=instructions[0].result) {
        error="rev36 supports a direct return of a zero-argument int call only"; return false;
    }
    const std::string callee=instructions[0].opcode.substr(5);
    if (callee==f.name) { error="recursive calls are not yet supported"; return false; }
    unsigned argument = 0;
    if (!instructions[0].operand1.empty()) {
        if (instructions[0].operand1.find(',') != std::string::npos ||
            !parseInteger(instructions[0].operand1, argument)) {
            error="rev37 supports one integer literal call argument only"; return false;
        }
    }
    e.reserveCallFrame(); // Win64 32-byte shadow space plus 8-byte alignment.
    if (!instructions[0].operand1.empty()) e.movEcxImmediate(argument);
    if (!instructions[0].debugLocation.file.empty() && instructions[0].debugLocation.line>0)
        e.markSource(instructions[0].debugLocation.file,
                     static_cast<unsigned>(instructions[0].debugLocation.line),1);
    coff::Relocation r;
    r.offset=e.callRel32Placeholder();
    r.symbol=callee;
    relocations.push_back(r);
    e.releaseCallFrame();
    e.ret();
    error.clear(); return true;
}
}}
