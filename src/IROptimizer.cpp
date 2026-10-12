#include "IR.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <map>
#include <ostream>
#include <sstream>

namespace shalimar {
namespace {

std::string numberText(int value) {
    std::ostringstream output;
    output << value;
    return output.str();
}
void canonicalizeBlocks(IRFunction& function) {
    std::map<int, int> ids;
    for (size_t b = 0; b < function.blocks.size(); ++b) ids[function.blocks[b].id] = static_cast<int>(b);
    for (size_t b = 0; b < function.blocks.size(); ++b) {
        IRBasicBlock& block = function.blocks[b];
        block.id = static_cast<int>(b);
        for (size_t e = 0; e < block.successors.size(); ++e) {
            std::map<int, int>::const_iterator found = ids.find(block.successors[e]);
            if (found != ids.end()) block.successors[e] = found->second;
        }
        for (size_t i = 0; i < block.instructions.size(); ++i) {
            IRInstruction& in = block.instructions[i];
            if (in.opcode == "jump" && block.successors.size() == 1)
                in.operand1 = "bb" + numberText(block.successors[0]);
            else if (in.opcode == "branch" && block.successors.size() == 2)
                in.operand2 = "bb" + numberText(block.successors[0]) + ", bb" + numberText(block.successors[1]);
        }
    }
    std::vector<IRLoopInduction> loops;
    for (size_t i = 0; i < function.inductionVariables.size(); ++i) {
        IRLoopInduction loop = function.inductionVariables[i];
        if (!ids.count(loop.preheaderBlock) || !ids.count(loop.headerBlock) || !ids.count(loop.stepBlock)) continue;
        loop.preheaderBlock = ids[loop.preheaderBlock]; loop.headerBlock = ids[loop.headerBlock]; loop.stepBlock = ids[loop.stepBlock];
        loops.push_back(loop);
    }
    function.inductionVariables.swap(loops);
    std::vector<IRStrengthReductionCandidate> candidates;
    for (size_t i = 0; i < function.strengthReductionCandidates.size(); ++i) {
        IRStrengthReductionCandidate candidate = function.strengthReductionCandidates[i];
        if (!ids.count(candidate.blockId)) continue;
        candidate.blockId = ids[candidate.blockId]; candidates.push_back(candidate);
    }
    function.strengthReductionCandidates.swap(candidates);
}
bool parseInteger(const std::string& text, long& value) {
    if (text.empty()) return false;
    errno = 0;
    char* end = 0;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (errno == ERANGE || end == text.c_str() || !end || *end != '\0' ||
        parsed < INT_MIN || parsed > INT_MAX) return false;
    value = parsed;
    return true;
}

std::string integerText(long value) {
    std::ostringstream output;
    output << value;
    return output.str();
}

bool isIdentifierCharacter(char value) {
    return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
           (value >= '0' && value <= '9') || value == '_' || value == '%';
}

std::string substituteConstants(const std::string& text,
                                const std::map<std::string, std::string>& constants) {
    std::string result;
    size_t index = 0;
    while (index < text.size()) {
        if (text[index] == '"') {
            const size_t start = index++;
            while (index < text.size()) {
                if (text[index] == '\\' && index + 1 < text.size()) {
                    index += 2;
                } else if (text[index++] == '"') {
                    break;
                }
            }
            result.append(text, start, index - start);
        } else if (isIdentifierCharacter(text[index]) &&
                   !((text[index] >= '0' && text[index] <= '9'))) {
            const size_t start = index++;
            while (index < text.size() && isIdentifierCharacter(text[index])) ++index;
            const std::string token = text.substr(start, index - start);
            std::map<std::string, std::string>::const_iterator found = constants.find(token);
            result += found == constants.end() ? token : found->second;
        } else {
            result += text[index++];
        }
    }
    return result;
}

bool isTemporary(const std::string& name) {
    return name.size() > 2 && name[0] == '%' && name[1] == 't';
}

bool foldIntegerOperation(const std::string& opcode, long left, long right,
                          long& result) {
    long double wide = 0.0L;
    if (opcode == "binary +") wide = static_cast<long double>(left) + right;
    else if (opcode == "binary -") wide = static_cast<long double>(left) - right;
    else if (opcode == "binary *") wide = static_cast<long double>(left) * right;
    else if (opcode == "binary /") {
        if (right == 0 || (left == INT_MIN && right == -1)) return false;
        result = left / right;
        return true;
    } else if (opcode == "binary %") {
        if (right == 0 || (left == INT_MIN && right == -1)) return false;
        result = left % right;
        return true;
    } else if (opcode == "binary =" || opcode == "binary ==") result = left == right;
    else if (opcode == "binary !=") result = left != right;
    else if (opcode == "binary <") result = left < right;
    else if (opcode == "binary >") result = left > right;
    else if (opcode == "binary <=") result = left <= right;
    else if (opcode == "binary >=") result = left >= right;
    else if (opcode == "binary &" || opcode == "binary &&")
        result = (left != 0 && right != 0);
    else if (opcode == "binary |" || opcode == "binary ||")
        result = (left != 0 || right != 0);
    else return false;

    if (opcode == "binary +" || opcode == "binary -" || opcode == "binary *") {
        if (wide < static_cast<long double>(INT_MIN) ||
            wide > static_cast<long double>(INT_MAX)) return false;
        const long converted = static_cast<long>(wide);
        if (static_cast<long double>(converted) != wide) return false;
        result = converted;
    }
    return true;
}

bool isPureInstruction(const IRInstruction& instruction) {
    return instruction.opcode == "constant" ||
           instruction.opcode.compare(0, 7, "binary ") == 0 ||
           instruction.opcode.compare(0, 6, "unary ") == 0 ||
           instruction.opcode == "for.test";
}

void findBranchTargets(const std::string& text, int& first, int& second) {
    first = -1;
    second = -1;
    size_t position = text.find("bb");
    if (position == std::string::npos) return;
    char* end = 0;
    const long firstValue = std::strtol(text.c_str() + position + 2, &end, 10);
    if (end == text.c_str() + position + 2 || firstValue < 0 || firstValue > INT_MAX) return;
    first = static_cast<int>(firstValue);
    position = text.find("bb", static_cast<size_t>(end - text.c_str()));
    if (position == std::string::npos) return;
    const long secondValue = std::strtol(text.c_str() + position + 2, &end, 10);
    if (end != text.c_str() + position + 2 && secondValue >= 0 && secondValue <= INT_MAX)
        second = static_cast<int>(secondValue);
}

bool blockIdExists(const std::vector<IRBasicBlock>& blocks, int id) {
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].id == id) return true;
    }
    return false;
}

bool isSafeInvariantOperation(const IRInstruction& instruction) {
    if (instruction.opcode == "unary !") return true;
    return instruction.opcode == "binary ==" || instruction.opcode == "binary !=" ||
           instruction.opcode == "binary <" || instruction.opcode == "binary >" ||
           instruction.opcode == "binary <=" || instruction.opcode == "binary >=" ||
           instruction.opcode == "binary &" || instruction.opcode == "binary |" ||
           instruction.opcode == "binary &&" || instruction.opcode == "binary ||";
}

bool operandIsInvariant(const std::string& operand,
                        const std::map<std::string, bool>& modified,
                        const std::map<std::string, bool>& loopTemporaries,
                        const std::map<std::string, bool>& invariantTemporaries,
                        bool loopHasCall) {
    if (operand.empty()) return false;
    long ignored = 0;
    if (parseInteger(operand, ignored)) return true;
    if (isTemporary(operand)) {
        return loopTemporaries.find(operand) == loopTemporaries.end() ||
               invariantTemporaries.find(operand) != invariantTemporaries.end();
    }
    if (loopHasCall || modified.find(operand) != modified.end()) return false;
    return operand.find_first_of(" ,\t\n\r\"") == std::string::npos;
}

int blockIndex(const std::vector<IRBasicBlock>& blocks, int id);
// Block numbering is layout, not dominance: nested if joins can have backward
// numbered edges without a loop. A natural back edge requires its target to
// dominate the source, so removing the target must disconnect the source.
bool reachableWithout(const IRFunction& function, int source, int excluded) {
    if (function.blocks.empty() || excluded == 0) return false;
    std::vector<bool> seen(function.blocks.size(), false);
    std::vector<int> pending(1, 0);
    while (!pending.empty()) {
        const int current = pending.back(); pending.pop_back();
        if (current == excluded || seen[static_cast<size_t>(current)]) continue;
        if (current == source) return true;
        seen[static_cast<size_t>(current)] = true;
        const IRBasicBlock& block = function.blocks[static_cast<size_t>(current)];
        for (size_t i = 0; i < block.successors.size(); ++i) {
            const int next = blockIndex(function.blocks, block.successors[i]);
            if (next >= 0) pending.push_back(next);
        }
    }
    return false;
}
bool naturalBackEdge(const IRFunction& function, int source, int header) {
    return reachableWithout(function, source, -1) && !reachableWithout(function, source, header);
}

bool instructionWritesVariable(const IRInstruction& instruction) {
    if (instruction.result.empty() || isTemporary(instruction.result)) return false;
    if (instruction.opcode == "declare" || instruction.opcode == "initialize" ||
        instruction.opcode == "assign =" || instruction.opcode == "assign :" ||
        instruction.opcode == "for.step" || instruction.opcode == "multi.assign") {
        return true;
    }
    return instruction.opcode.compare(0, 7, "assign ") == 0;
}

/*
 * Recognize canonical, constant-stride for loops for analysis and future strength
 * reduction. This pass records facts only; it does not rewrite loop semantics.
 * It deliberately accepts only literal start, step, and limit values and rejects
 * loops that write the induction variable anywhere except the generated for.step.
 */
void analyzeInductionVariables(IRFunction& function) {
    function.inductionVariables.clear();
    const size_t blockCount = function.blocks.size();
    if (blockCount < 3) return;

    std::vector<std::vector<int> > predecessors(blockCount);
    for (size_t b = 0; b < blockCount; ++b) {
        for (size_t s = 0; s < function.blocks[b].successors.size(); ++s) {
            const int target = blockIndex(function.blocks, function.blocks[b].successors[s]);
            if (target >= 0) predecessors[static_cast<size_t>(target)].push_back(static_cast<int>(b));
        }
    }

    std::map<std::string, bool> analyzedEdges;
    for (size_t source = 0; source < blockCount; ++source) {
        for (size_t edge = 0; edge < function.blocks[source].successors.size(); ++edge) {
            const int headerId = function.blocks[source].successors[edge];
            const int headerIndex = blockIndex(function.blocks, headerId);
            if (headerIndex < 0 || !naturalBackEdge(function, static_cast<int>(source), headerIndex)) continue;
            const std::string edgeKey = integerText(function.blocks[source].id) + ":" +
                                        integerText(headerId);
            if (analyzedEdges.find(edgeKey) != analyzedEdges.end()) continue;
            analyzedEdges[edgeKey] = true;

            const IRBasicBlock& header = function.blocks[static_cast<size_t>(headerIndex)];
            std::string inductionName;
            std::string limit;
            bool hasTest = false;
            for (size_t i = 0; i < header.instructions.size(); ++i) {
                const IRInstruction& instruction = header.instructions[i];
                if (instruction.opcode == "for.test") {
                    inductionName = instruction.operand1;
                    limit = instruction.operand2;
                    hasTest = true;
                    break;
                }
            }
            if (!hasTest || inductionName.empty()) continue;
            long parsedLimit = 0;
            if (!parseInteger(limit, parsedLimit)) continue;

            std::string increment;
            bool hasStep = false;
            const IRBasicBlock& stepBlock = function.blocks[source];
            for (size_t i = 0; i < stepBlock.instructions.size(); ++i) {
                const IRInstruction& instruction = stepBlock.instructions[i];
                if (instruction.opcode == "for.step" && instruction.result == inductionName) {
                    increment = instruction.operand1;
                    hasStep = true;
                    break;
                }
            }
            long parsedStep = 0;
            if (!hasStep || !parseInteger(increment, parsedStep) || parsedStep == 0) continue;

            std::vector<bool> inLoop(blockCount, false);
            std::vector<int> worklist;
            inLoop[static_cast<size_t>(headerIndex)] = true;
            inLoop[source] = true;
            if (static_cast<int>(source) != headerIndex) worklist.push_back(static_cast<int>(source));
            while (!worklist.empty()) {
                const int current = worklist.back();
                worklist.pop_back();
                for (size_t p = 0; p < predecessors[static_cast<size_t>(current)].size(); ++p) {
                    const int predecessor = predecessors[static_cast<size_t>(current)][p];
                    if (!inLoop[static_cast<size_t>(predecessor)]) {
                        inLoop[static_cast<size_t>(predecessor)] = true;
                        if (predecessor != headerIndex) worklist.push_back(predecessor);
                    }
                }
            }

            int preheaderIndex = -1;
            bool ambiguousPreheader = false;
            for (size_t p = 0; p < predecessors[static_cast<size_t>(headerIndex)].size(); ++p) {
                const int predecessor = predecessors[static_cast<size_t>(headerIndex)][p];
                if (!inLoop[static_cast<size_t>(predecessor)]) {
                    if (preheaderIndex >= 0 && preheaderIndex != predecessor)
                        ambiguousPreheader = true;
                    preheaderIndex = predecessor;
                }
            }
            if (preheaderIndex < 0 || ambiguousPreheader) continue;

            std::string initialValue;
            bool hasInitialValue = false;
            const IRBasicBlock& preheader = function.blocks[static_cast<size_t>(preheaderIndex)];
            for (size_t i = 0; i < preheader.instructions.size(); ++i) {
                const IRInstruction& instruction = preheader.instructions[i];
                if (instruction.opcode == "assign =" && instruction.result == inductionName) {
                    initialValue = instruction.operand1;
                    hasInitialValue = true;
                }
            }
            long parsedStart = 0;
            if (!hasInitialValue || !parseInteger(initialValue, parsedStart)) continue;

            bool safe = true;
            for (size_t b = 0; b < blockCount && safe; ++b) {
                if (!inLoop[b]) continue;
                const std::vector<IRInstruction>& instructions = function.blocks[b].instructions;
                for (size_t i = 0; i < instructions.size(); ++i) {
                    const IRInstruction& instruction = instructions[i];
                    if (instruction.result == inductionName &&
                        instruction.opcode != "for.step" && instructionWritesVariable(instruction)) {
                        safe = false;
                        break;
                    }
                    if (instruction.opcode.compare(0, 5, "call ") == 0 ||
                        instruction.opcode == "multi.assign") {
                        /* Calls may mutate an aliased induction variable. */
                        safe = false;
                        break;
                    }
                }
            }
            if (!safe) continue;

            function.inductionVariables.push_back(IRLoopInduction(
                preheader.id, header.id, stepBlock.id, inductionName,
                initialValue, increment, limit));
        }
    }
}

typedef std::map<std::string, std::string> ConstantMap;

/*
 * Perform a deliberately narrow, proven-safe strength-reduction rewrite.
 * It only accepts a single unconditional loop body block, a literal nonzero
 * trip count, a literal stride/bound/factor, and an induction range for which
 * both the original product and the recurrence (including the final update)
 * remain within the Shalimar int range. The IR is diagnostic-only today; this
 * rewrites the Release IR representation, not the AST-to-C native backend.
 */
void analyzeStrengthReductionCandidates(IRFunction& function) {
    function.strengthReductionCandidates.clear();
    int nextRecurrence = 0;

    for (size_t loopIndex = 0; loopIndex < function.inductionVariables.size(); ++loopIndex) {
        const IRLoopInduction& loop = function.inductionVariables[loopIndex];
        long parsedStart = 0;
        long parsedStep = 0;
        long parsedLimit = 0;
        if (!parseInteger(loop.start, parsedStart) ||
            !parseInteger(loop.step, parsedStep) ||
            !parseInteger(loop.limit, parsedLimit) || parsedStep == 0 ||
            parsedStart < INT_MIN || parsedStart > INT_MAX ||
            parsedStep < INT_MIN || parsedStep > INT_MAX ||
            parsedLimit < INT_MIN || parsedLimit > INT_MAX) {
            continue;
        }

        const double start = static_cast<double>(parsedStart);
        const double step = static_cast<double>(parsedStep);
        const double limit = static_cast<double>(parsedLimit);
        if ((step > 0.0 && start > limit) || (step < 0.0 && start < limit)) continue;

        double last = start;
        if (step > 0.0) last += std::floor((limit - start) / step) * step;
        else last += std::floor((start - limit) / (-step)) * step;
        const double next = last + step;
        if (next < static_cast<double>(INT_MIN) ||
            next > static_cast<double>(INT_MAX)) continue;

        const int preheaderIndex = blockIndex(function.blocks, loop.preheaderBlock);
        const int headerIndex = blockIndex(function.blocks, loop.headerBlock);
        const int stepIndex = blockIndex(function.blocks, loop.stepBlock);
        if (preheaderIndex < 0 || headerIndex < 0 || stepIndex < 0) continue;

        IRBasicBlock& header = function.blocks[static_cast<size_t>(headerIndex)];
        if (header.successors.size() != 2) continue;
        int bodyIndex = -1;
        for (size_t successor = 0; successor < header.successors.size(); ++successor) {
            const int index = blockIndex(function.blocks, header.successors[successor]);
            if (index < 0) continue;
            const IRBasicBlock& candidate = function.blocks[static_cast<size_t>(index)];
            if (candidate.id != loop.stepBlock && candidate.id != loop.preheaderBlock) {
                bodyIndex = index;
                break;
            }
        }
        if (bodyIndex < 0) continue;
        IRBasicBlock& body = function.blocks[static_cast<size_t>(bodyIndex)];
        if (body.successors.size() != 1 || body.successors[0] != loop.stepBlock) continue;

        int predecessorCount = 0;
        for (size_t b = 0; b < function.blocks.size(); ++b) {
            for (size_t edge = 0; edge < function.blocks[b].successors.size(); ++edge) {
                if (function.blocks[b].successors[edge] == body.id) ++predecessorCount;
            }
        }
        if (predecessorCount != 1) continue;

        for (size_t instructionIndex = 0; instructionIndex < body.instructions.size();
             ++instructionIndex) {
            IRInstruction& instruction = body.instructions[instructionIndex];
            if (instruction.opcode != "binary *" || instruction.result.empty() ||
                !isTemporary(instruction.result)) continue;

            long factor = 0;
            std::string factorText;
            if (instruction.operand1 == loop.variable &&
                parseInteger(instruction.operand2, factor)) {
                factorText = instruction.operand2;
            } else if (instruction.operand2 == loop.variable &&
                       parseInteger(instruction.operand1, factor)) {
                factorText = instruction.operand1;
            } else {
                continue;
            }
            if (factor == 0 || factor < INT_MIN || factor > INT_MAX) continue;

            /* The product temporary must be used only inside this unconditional body. */
            bool outsideUse = false;
            for (size_t b = 0; b < function.blocks.size() && !outsideUse; ++b) {
                if (static_cast<int>(b) == bodyIndex) continue;
                for (size_t n = 0; n < function.blocks[b].instructions.size(); ++n) {
                    const IRInstruction& use = function.blocks[b].instructions[n];
                    if (use.operand1 == instruction.result || use.operand2 == instruction.result) {
                        outsideUse = true;
                        break;
                    }
                }
            }
            if (outsideUse) continue;

            const double firstProduct = start * static_cast<double>(factor);
            const double nextProduct = next * static_cast<double>(factor);
            const double delta = step * static_cast<double>(factor);
            if (firstProduct < static_cast<double>(INT_MIN) ||
                firstProduct > static_cast<double>(INT_MAX) ||
                nextProduct < static_cast<double>(INT_MIN) ||
                nextProduct > static_cast<double>(INT_MAX) ||
                delta < static_cast<double>(INT_MIN) ||
                delta > static_cast<double>(INT_MAX)) continue;

            IRBasicBlock& stepBlock = function.blocks[static_cast<size_t>(stepIndex)];
            size_t stepInstruction = stepBlock.instructions.size();
            for (size_t n = 0; n < stepBlock.instructions.size(); ++n) {
                if (stepBlock.instructions[n].opcode == "for.step" &&
                    stepBlock.instructions[n].result == loop.variable) {
                    stepInstruction = n + 1;
                    break;
                }
            }
            if (stepInstruction > stepBlock.instructions.size()) continue;

            const std::string recurrence = "%sr" + numberText(nextRecurrence++);
            IRBasicBlock& preheader = function.blocks[static_cast<size_t>(preheaderIndex)];
            IRInstruction initial("binary *", recurrence, loop.start, factorText,
                                  instruction.sourceLine);
            size_t insertion = preheader.instructions.size();
            if (insertion > 0 && preheader.instructions[insertion - 1].opcode == "jump") --insertion;
            preheader.instructions.insert(preheader.instructions.begin() + insertion, initial);
            IRInstruction update("binary +", recurrence, recurrence, integerText(delta),
                                instruction.sourceLine);
            stepBlock.instructions.insert(stepBlock.instructions.begin() + stepInstruction, update);

            instruction.opcode = "assign =";
            instruction.operand1 = recurrence;
            instruction.operand2.clear();
            function.strengthReductionCandidates.push_back(
                IRStrengthReductionCandidate(instruction.result, loop.variable, factorText,
                                             integerText(delta), body.id, true, recurrence));
        }
    }
}

int blockIndex(const std::vector<IRBasicBlock>& blocks, int id) {
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

/*
 * Conservative loop-invariant code motion for total comparison/logical operations.
 * Arithmetic is deliberately excluded because signed overflow can be undefined in
 * the generated C, and moving it can change whether it executes on a zero-trip loop.
 * The IR is diagnostic today; this pass prepares its Release representation only.
 */
bool moveLoopInvariantComparisons(IRFunction& function) {
    const size_t blockCount = function.blocks.size();
    if (blockCount < 3) return false;

    std::vector<std::vector<int> > predecessors(blockCount);
    for (size_t b = 0; b < blockCount; ++b) {
        for (size_t s = 0; s < function.blocks[b].successors.size(); ++s) {
            const int target = blockIndex(function.blocks, function.blocks[b].successors[s]);
            if (target >= 0) predecessors[static_cast<size_t>(target)].push_back(static_cast<int>(b));
        }
    }

    bool anyChanged = false;
    std::map<std::string, bool> processedBackEdges;
    for (size_t source = 0; source < blockCount; ++source) {
        for (size_t edge = 0; edge < function.blocks[source].successors.size(); ++edge) {
            const int headerIndex = blockIndex(function.blocks,
                                               function.blocks[source].successors[edge]);
            if (headerIndex < 0 || !naturalBackEdge(function, static_cast<int>(source), headerIndex))
                continue;
            const std::string edgeKey = integerText(function.blocks[source].id) + ":" +
                                        integerText(function.blocks[static_cast<size_t>(headerIndex)].id);
            if (processedBackEdges.find(edgeKey) != processedBackEdges.end()) continue;
            processedBackEdges[edgeKey] = true;

            std::vector<bool> inLoop(blockCount, false);
            std::vector<int> worklist;
            inLoop[static_cast<size_t>(headerIndex)] = true;
            inLoop[source] = true;
            if (static_cast<int>(source) != headerIndex) worklist.push_back(static_cast<int>(source));
            while (!worklist.empty()) {
                const int current = worklist.back();
                worklist.pop_back();
                for (size_t p = 0; p < predecessors[static_cast<size_t>(current)].size(); ++p) {
                    const int predecessor = predecessors[static_cast<size_t>(current)][p];
                    if (!inLoop[static_cast<size_t>(predecessor)]) {
                        inLoop[static_cast<size_t>(predecessor)] = true;
                        if (predecessor != headerIndex) worklist.push_back(predecessor);
                    }
                }
            }

            int preheaderIndex = -1;
            bool multiplePreheaders = false;
            for (size_t p = 0; p < predecessors[static_cast<size_t>(headerIndex)].size(); ++p) {
                const int predecessor = predecessors[static_cast<size_t>(headerIndex)][p];
                if (!inLoop[static_cast<size_t>(predecessor)]) {
                    if (preheaderIndex >= 0 && preheaderIndex != predecessor) multiplePreheaders = true;
                    preheaderIndex = predecessor;
                }
            }
            if (preheaderIndex < 0 || multiplePreheaders) continue;

            std::map<std::string, bool> modified;
            std::map<std::string, bool> loopTemporaries;
            bool loopHasCall = false;
            for (size_t b = 0; b < blockCount; ++b) {
                if (!inLoop[b]) continue;
                const std::vector<IRInstruction>& instructions = function.blocks[b].instructions;
                for (size_t i = 0; i < instructions.size(); ++i) {
                    const IRInstruction& instruction = instructions[i];
                    if (instructionWritesVariable(instruction)) modified[instruction.result] = true;
                    if (isTemporary(instruction.result)) loopTemporaries[instruction.result] = true;
                    if (instruction.opcode.compare(0, 5, "call ") == 0 ||
                        instruction.opcode == "multi.assign")
                        loopHasCall = true;
                }
            }

            std::map<std::string, bool> invariantTemporaries;
            bool movedInIteration = true;
            while (movedInIteration) {
                movedInIteration = false;
                for (size_t b = 0; b < blockCount; ++b) {
                    if (!inLoop[b]) continue;
                    std::vector<IRInstruction>& instructions = function.blocks[b].instructions;
                    std::vector<IRInstruction> kept;
                    for (size_t i = 0; i < instructions.size(); ++i) {
                        const IRInstruction& instruction = instructions[i];
                        const bool candidate = isTemporary(instruction.result) &&
                            isSafeInvariantOperation(instruction) &&
                            operandIsInvariant(instruction.operand1, modified, loopTemporaries,
                                               invariantTemporaries, loopHasCall) &&
                            (instruction.opcode.compare(0, 6, "unary ") == 0 ||
                             operandIsInvariant(instruction.operand2, modified, loopTemporaries,
                                                invariantTemporaries, loopHasCall));
                        if (candidate) {
                            invariantTemporaries[instruction.result] = true;
                            IRBasicBlock& preheader = function.blocks[static_cast<size_t>(preheaderIndex)];
                            size_t insertion = preheader.instructions.size();
                            if (insertion > 0 &&
                                (preheader.instructions[insertion - 1].opcode == "jump" ||
                                 preheader.instructions[insertion - 1].opcode == "branch")) {
                                --insertion;
                            }
                            preheader.instructions.insert(preheader.instructions.begin() + insertion,
                                                          instruction);
                            movedInIteration = true;
                            anyChanged = true;
                        } else {
                            kept.push_back(instruction);
                        }
                    }
                    instructions.swap(kept);
                }
            }
        }
    }
    return anyChanged;
}

void foldInstruction(IRInstruction& instruction) {
    if (instruction.opcode.compare(0, 7, "binary ") == 0) {
        long left = 0;
        long right = 0;
        long result = 0;
        if (parseInteger(instruction.operand1, left) &&
            parseInteger(instruction.operand2, right) &&
            foldIntegerOperation(instruction.opcode, left, right, result)) {
            instruction.opcode = "constant";
            instruction.operand1 = integerText(result);
            instruction.operand2.clear();
        }
    } else if (instruction.opcode.compare(0, 6, "unary ") == 0) {
        long operand = 0;
        if (!parseInteger(instruction.operand1, operand)) return;
        bool folded = true;
        long result = operand;
        const std::string operation = instruction.opcode.substr(6);
        if (operation == "-") {
            if (operand == INT_MIN) folded = false;
            else result = -operand;
        } else if (operation == "!") {
            result = operand == 0 ? 1 : 0;
        } else {
            folded = false;
        }
        if (folded) {
            instruction.opcode = "constant";
            instruction.operand1 = integerText(result);
            instruction.operand2.clear();
        }
    }
}

void updateConstantState(const IRInstruction& instruction, ConstantMap& constants) {
    if (instruction.opcode == "declare" ||
        instruction.opcode.compare(0, 7, "global ") == 0) {
        constants.erase(instruction.result);
    } else if (instruction.opcode == "initialize" ||
               instruction.opcode == "global.initialize" ||
               instruction.opcode == "assign =" ||
               instruction.opcode == "assign :") {
        long value = 0;
        if (!instruction.result.empty() &&
            instruction.result.find('%') == std::string::npos &&
            instruction.result.find('[') == std::string::npos &&
            parseInteger(instruction.operand1, value)) {
            constants[instruction.result] = integerText(value);
        } else if (!instruction.result.empty() &&
                   instruction.result.find('%') == std::string::npos) {
            constants.erase(instruction.result);
        } else if (instruction.opcode == "assign :") {
            constants.clear();
        }
    } else if (instruction.opcode.compare(0, 7, "assign ") == 0) {
        constants.clear();
    } else if (instruction.opcode.compare(0, 5, "call ") == 0 ||
               instruction.opcode == "multi.assign" ||
               instruction.opcode == "array.element" ||
               instruction.opcode == "array.begin" ||
               instruction.opcode == "index" ||
               instruction.opcode.compare(0, 12, "index.store ") == 0 ||
               instruction.opcode == "for.step") {
        constants.clear();
    }

    if (isTemporary(instruction.result) && instruction.opcode == "constant") {
        constants[instruction.result] = instruction.operand1;
    } else if (isTemporary(instruction.result) &&
               (instruction.opcode.compare(0, 7, "binary ") == 0 ||
                instruction.opcode.compare(0, 6, "unary ") == 0)) {
        constants.erase(instruction.result);
    }
}

void transferBlock(const IRBasicBlock& block, const ConstantMap& incoming,
                   ConstantMap& outgoing) {
    outgoing = incoming;
    for (size_t i = 0; i < block.instructions.size(); ++i) {
        IRInstruction instruction = block.instructions[i];
        if (instruction.opcode.compare(0, 5, "call ") == 0) outgoing.clear();
        instruction.operand1 = substituteConstants(instruction.operand1, outgoing);
        instruction.operand2 = substituteConstants(instruction.operand2, outgoing);
        foldInstruction(instruction);
        updateConstantState(instruction, outgoing);
    }
}

void reachableBlocks(const std::vector<IRBasicBlock>& blocks,
                     std::vector<bool>& reachable) {
    reachable.assign(blocks.size(), false);
    if (blocks.empty()) return;
    std::vector<int> worklist;
    worklist.push_back(blocks[0].id);
    while (!worklist.empty()) {
        const int id = worklist.back();
        worklist.pop_back();
        const int index = blockIndex(blocks, id);
        if (index < 0 || reachable[static_cast<size_t>(index)]) continue;
        reachable[static_cast<size_t>(index)] = true;
        const IRBasicBlock& block = blocks[static_cast<size_t>(index)];
        for (size_t i = 0; i < block.successors.size(); ++i)
            worklist.push_back(block.successors[i]);
    }
}

bool propagateAcrossBlocks(IRFunction& function) {
    if (function.blocks.empty()) return false;
    const size_t originalBlockCount = function.blocks.size();
    std::vector<bool> reachable;
    reachableBlocks(function.blocks, reachable);

    std::vector<std::vector<size_t> > predecessors(function.blocks.size());
    for (size_t b = 0; b < function.blocks.size(); ++b) {
        if (!reachable[b]) continue;
        for (size_t s = 0; s < function.blocks[b].successors.size(); ++s) {
            const int target = blockIndex(function.blocks, function.blocks[b].successors[s]);
            if (target >= 0 && reachable[static_cast<size_t>(target)])
                predecessors[static_cast<size_t>(target)].push_back(b);
        }
    }

    std::vector<ConstantMap> incoming(function.blocks.size());
    std::vector<ConstantMap> outgoing(function.blocks.size());
    bool dataflowChanged = true;
    while (dataflowChanged) {
        dataflowChanged = false;
        for (size_t b = 0; b < function.blocks.size(); ++b) {
            ConstantMap nextIncoming;
            if (b != 0 && reachable[b] && !predecessors[b].empty()) {
                nextIncoming = outgoing[predecessors[b][0]];
                for (size_t p = 1; p < predecessors[b].size(); ++p) {
                    const ConstantMap& other = outgoing[predecessors[b][p]];
                    for (ConstantMap::iterator value = nextIncoming.begin();
                         value != nextIncoming.end();) {
                        ConstantMap::const_iterator match = other.find(value->first);
                        if (match == other.end() || match->second != value->second)
                            nextIncoming.erase(value++);
                        else
                            ++value;
                    }
                }
            }
            ConstantMap nextOutgoing;
            if (reachable[b]) transferBlock(function.blocks[b], nextIncoming, nextOutgoing);
            if (nextIncoming != incoming[b] || nextOutgoing != outgoing[b]) {
                incoming[b].swap(nextIncoming);
                outgoing[b].swap(nextOutgoing);
                dataflowChanged = true;
            }
        }
    }

    bool changed = false;
    for (size_t b = 0; b < function.blocks.size(); ++b) {
        if (!reachable[b]) continue;
        ConstantMap constants = incoming[b];
        IRBasicBlock& block = function.blocks[b];
        for (size_t i = 0; i < block.instructions.size(); ++i) {
            IRInstruction& instruction = block.instructions[i];
            const std::string oldOpcode = instruction.opcode;
            const std::string oldFirst = instruction.operand1;
            const std::string oldSecond = instruction.operand2;
            if (instruction.opcode.compare(0, 5, "call ") == 0) constants.clear();
            instruction.operand1 = substituteConstants(instruction.operand1, constants);
            instruction.operand2 = substituteConstants(instruction.operand2, constants);
            foldInstruction(instruction);
            if (instruction.opcode == "branch") {
                long condition = 0;
                if (parseInteger(instruction.operand1, condition)) {
                    int yesTarget = -1;
                    int noTarget = -1;
                    findBranchTargets(instruction.operand2, yesTarget, noTarget);
                    const int selected = condition != 0 ? yesTarget : noTarget;
                    if (selected >= 0) {
                        instruction.opcode = "jump";
                        instruction.operand1 = "bb" + integerText(selected);
                        instruction.operand2.clear();
                        block.successors.clear();
                        block.successors.push_back(selected);
                    }
                }
            }
            if (oldOpcode != instruction.opcode || oldFirst != instruction.operand1 ||
                oldSecond != instruction.operand2) changed = true;
            updateConstantState(instruction, constants);
        }
    }

    reachableBlocks(function.blocks, reachable);
    std::vector<IRBasicBlock> kept;
    for (size_t b = 0; b < function.blocks.size(); ++b) {
        if (reachable[b]) kept.push_back(function.blocks[b]);
    }
    if (kept.size() != originalBlockCount) {
        function.blocks.swap(kept);
        canonicalizeBlocks(function);
        changed = true;
    }
    return changed;
}

} // namespace

void IROptimizer::optimize(IRProgram& program) const {
    for (size_t i = 0; i < program.globals.size(); ++i) {
        IRInstruction& instruction = program.globals[i];
        if (!instruction.operand1.empty()) {
            long value = 0;
            if (parseInteger(instruction.operand1, value)) {
                instruction.operand1 = integerText(value);
            }
        }
    }
    for (size_t i = 0; i < program.functions.size(); ++i) {
        optimizeFunction(program.functions[i]);
    }
    // Optimization rewrites operand spellings. Refresh the structured
    // descriptors atomically before the post-optimization IR verifier runs.
    for (size_t i = 0; i < program.globals.size(); ++i) {
        IRInstruction& in = program.globals[i];
        in.resultValue = IROperand(in.result);
        in.firstValue = IROperand(in.operand1);
        in.secondValue = IROperand(in.operand2);
    }
    for (size_t f = 0; f < program.functions.size(); ++f)
        for (size_t b = 0; b < program.functions[f].blocks.size(); ++b)
            for (size_t j = 0; j < program.functions[f].blocks[b].instructions.size(); ++j) {
                IRInstruction& in = program.functions[f].blocks[b].instructions[j];
                in.resultValue = IROperand(in.result);
                in.firstValue = IROperand(in.operand1);
                in.secondValue = IROperand(in.operand2);
            }
}

void IROptimizer::optimizeFunction(IRFunction& function) const {
    /* Re-run data-flow after branch folding removes predecessor paths. */
    const size_t maximumPasses = function.blocks.size() + 2;
    for (size_t pass = 0; pass < maximumPasses; ++pass) {
        if (!propagateAcrossBlocks(function)) break;
    }

    moveLoopInvariantComparisons(function);
    analyzeInductionVariables(function);
    analyzeStrengthReductionCandidates(function);

    std::map<std::string, std::string> constants;

    for (size_t b = 0; b < function.blocks.size(); ++b) {
        IRBasicBlock& block = function.blocks[b];
        constants.clear();
        for (size_t i = 0; i < block.instructions.size(); ++i) {
            IRInstruction& instruction = block.instructions[i];
            if (instruction.opcode.compare(0, 5, "call ") == 0) constants.clear();
            instruction.operand1 = substituteConstants(instruction.operand1, constants);
            instruction.operand2 = substituteConstants(instruction.operand2, constants);

            if (instruction.opcode.compare(0, 7, "binary ") == 0) {
                long left = 0;
                long right = 0;
                long result = 0;
                if (parseInteger(instruction.operand1, left) &&
                    parseInteger(instruction.operand2, right) &&
                    foldIntegerOperation(instruction.opcode, left, right, result)) {
                    instruction.opcode = "constant";
                    instruction.operand1 = integerText(result);
                    instruction.operand2.clear();
                }
            } else if (instruction.opcode.compare(0, 6, "unary ") == 0) {
                long operand = 0;
                if (parseInteger(instruction.operand1, operand)) {
                    bool folded = true;
                    long result = operand;
                    const std::string operation = instruction.opcode.substr(6);
                    if (operation == "-") {
                        if (operand == INT_MIN) folded = false;
                        else result = -operand;
                    } else if (operation == "!") {
                        result = operand == 0 ? 1 : 0;
                    } else {
                        folded = false;
                    }
                    if (folded) {
                        instruction.opcode = "constant";
                        instruction.operand1 = integerText(result);
                        instruction.operand2.clear();
                    }
                }
            }

            if (instruction.opcode == "branch") {
                long condition = 0;
                if (parseInteger(instruction.operand1, condition)) {
                    int yesTarget = -1;
                    int noTarget = -1;
                    findBranchTargets(instruction.operand2, yesTarget, noTarget);
                    const int selected = condition != 0 ? yesTarget : noTarget;
                    if (selected >= 0) {
                        instruction.opcode = "jump";
                        instruction.operand1 = "bb" + integerText(selected);
                        instruction.operand2.clear();
                        block.successors.clear();
                        block.successors.push_back(selected);
                    }
                }
            }

            if (instruction.opcode == "declare" || instruction.opcode.compare(0, 7, "global ") == 0) {
                constants.erase(instruction.result);
            } else if (instruction.opcode == "initialize" ||
                       instruction.opcode == "global.initialize" ||
                       instruction.opcode == "assign =" ||
                       instruction.opcode == "assign :") {
                long value = 0;
                if (parseInteger(instruction.operand1, value) &&
                    !instruction.result.empty() &&
                    instruction.result.find('%') == std::string::npos &&
                    instruction.result.find('[') == std::string::npos) {
                    constants[instruction.result] = integerText(value);
                } else if (!instruction.result.empty() &&
                           instruction.result.find('%') == std::string::npos) {
                    constants.erase(instruction.result);
                } else if (instruction.opcode == "assign :") {
                    constants.clear();
                }
            } else if (instruction.opcode.compare(0, 7, "assign ") == 0) {
                constants.erase(instruction.result);
                constants.clear();
            } else if (instruction.opcode.compare(0, 5, "call ") == 0 ||
                       instruction.opcode == "array.element" ||
                       instruction.opcode == "array.begin" ||
                       instruction.opcode == "for.step" ||
                       instruction.opcode == "multi.assign") {
                constants.clear();
            }

            if (isTemporary(instruction.result) && instruction.opcode == "constant") {
                constants[instruction.result] = instruction.operand1;
            } else if (isTemporary(instruction.result) &&
                       (instruction.opcode.compare(0, 7, "binary ") == 0 ||
                        instruction.opcode.compare(0, 6, "unary ") == 0)) {
                constants.erase(instruction.result);
            }
        }
    }

    /* Remove blocks made unreachable by constant branches. */
    if (!function.blocks.empty()) {
        std::vector<int> worklist;
        std::vector<int> reachable;
        worklist.push_back(function.blocks[0].id);
        while (!worklist.empty()) {
            const int id = worklist.back();
            worklist.pop_back();
            bool seen = false;
            for (size_t r = 0; r < reachable.size(); ++r) {
                if (reachable[r] == id) { seen = true; break; }
            }
            if (seen || !blockIdExists(function.blocks, id)) continue;
            reachable.push_back(id);
            for (size_t b = 0; b < function.blocks.size(); ++b) {
                if (function.blocks[b].id != id) continue;
                for (size_t s = 0; s < function.blocks[b].successors.size(); ++s)
                    worklist.push_back(function.blocks[b].successors[s]);
                break;
            }
        }
        std::vector<IRBasicBlock> kept;
        for (size_t b = 0; b < function.blocks.size(); ++b) {
            bool seen = false;
            for (size_t r = 0; r < reachable.size(); ++r) {
                if (reachable[r] == function.blocks[b].id) { seen = true; break; }
            }
            if (seen) kept.push_back(function.blocks[b]);
        }
        function.blocks.swap(kept);
        canonicalizeBlocks(function);
    }

    /* Repeatedly remove unused pure temporary computations. */
    bool changed = true;
    while (changed) {
        changed = false;
        std::map<std::string, bool> used;
        for (size_t b = 0; b < function.blocks.size(); ++b) {
            for (size_t i = 0; i < function.blocks[b].instructions.size(); ++i) {
                const IRInstruction& instruction = function.blocks[b].instructions[i];
                const std::string fields[2] = {instruction.operand1, instruction.operand2};
                for (int f = 0; f < 2; ++f) {
                    size_t position = 0;
                    while (position < fields[f].size()) {
                        if (fields[f][position] == '"') {
                            ++position;
                            while (position < fields[f].size()) {
                                if (fields[f][position] == '\\' && position + 1 < fields[f].size())
                                    position += 2;
                                else if (fields[f][position++] == '"') break;
                            }
                        } else if (fields[f][position] == '%') {
                            const size_t start = position++;
                            while (position < fields[f].size() && isIdentifierCharacter(fields[f][position]))
                                ++position;
                            used[fields[f].substr(start, position - start)] = true;
                        } else {
                            ++position;
                        }
                    }
                }
            }
        }
        for (size_t b = 0; b < function.blocks.size(); ++b) {
            std::vector<IRInstruction> kept;
            for (size_t i = 0; i < function.blocks[b].instructions.size(); ++i) {
                const IRInstruction& instruction = function.blocks[b].instructions[i];
                if (isTemporary(instruction.result) && isPureInstruction(instruction) &&
                    used.find(instruction.result) == used.end()) {
                    changed = true;
                } else {
                    kept.push_back(instruction);
                }
            }
            function.blocks[b].instructions.swap(kept);
        }
    }
}

} // namespace shalimar
