#include "backend/tms6747/IRAssembly.h"
#include "../legacy_port/Tms6747.h"
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <cfloat>
#include <map>
#include <set>
#include <sstream>
namespace shalimar { namespace tms6747 {
namespace {
std::string trim(const std::string& s) {
    std::size_t a = s.find_first_not_of(" \t");
    return a == std::string::npos ? "" : s.substr(a, s.find_last_not_of(" \t") - a + 1);
}
std::vector<std::string> split(const std::string& text) {
    std::vector<std::string> result; std::istringstream stream(text); std::string s;
    while (std::getline(stream, s, ',')) result.push_back(trim(s));
    return result;
}
bool integer(const std::string& s, int& value) {
    char* end = 0; errno = 0; long n = std::strtol(s.c_str(), &end, 10);
    if (s.empty() || errno || end == s.c_str() || *end || n < INT_MIN || n > INT_MAX) return false;
    value = static_cast<int>(n); return true;
}
bool scalar(const IRType& t) { return !t.arrayRank && !t.byReference && (t.name == "int" || t.name == "real"); }
struct Value {
    int slot; Slot type;
    Value() : slot(-1), type(Int) {}
    Value(int s, Slot t) : slot(s), type(t) {}
};
typedef std::map<std::string, Value> Values;
bool kind(const std::string& s, const Values& values, Slot& type) {
    Values::const_iterator it = values.find(s);
    if (it != values.end()) { type = it->second.type; return true; }
    int n;
    if (integer(s, n)) { type = Int; return true; }
    char* end = 0; errno = 0; double v = std::strtod(s.c_str(), &end);
    if (!s.empty() && !errno && end != s.c_str() && !*end && v == v && v <= DBL_MAX && v >= -DBL_MAX) { type = Real; return true; }
    return false;
}
bool load(Tms6747Emitter& e, const std::string& s, const Values& values, Slot desired) {
    Values::const_iterator it = values.find(s);
    if (it != values.end()) {
        e.loadSlot(it->second.type, it->second.slot);
        if (it->second.type == Int && desired == Real) {
            e.nativeInstruction("INTDP A4, A5:A4"); e.nativeInstruction("NOP 9");
        }
        return it->second.type == desired || (it->second.type == Int && desired == Real);
    }
    int n;
    if (integer(s, n)) { if (desired == Real) e.loadRealConstant(n); else e.loadIntConstant(n); return true; }
    Slot type;
    if (desired == Real && kind(s, values, type)) { e.loadRealConstant(std::strtod(s.c_str(), 0)); return true; }
    return false;
}
const IRFunction* find(const IRProgram& p, const std::string& name) {
    for (std::size_t i = 0; i < p.functions.size(); ++i) if (p.functions[i].name == name) return &p.functions[i];
    return 0;
}
typedef std::set<std::string> Initialized;
void transfer(const IRBasicBlock& block, Initialized& state) {
    for (std::size_t i = 0; i < block.instructions.size(); ++i) {
        const IRInstruction& in = block.instructions[i];
        if (in.result.empty()) continue;
        if (in.opcode.find("declare ") == 0) state.erase(in.result);
        else state.insert(in.result);
    }
}
// Must-initialization analysis: only values initialized on every reachable
// predecessor path may be loaded. Loop facts converge from the lattice top.
bool initializedReads(const IRFunction& f, const Values& values, std::string& error) {
    const std::size_t count = f.blocks.size();
    if (!count) { error = "missing entry block"; return false; }
    std::vector<bool> reachable(count, false); reachable[0] = true;
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t b = 0; b < count; ++b) if (reachable[b])
            for (std::size_t s = 0; s < f.blocks[b].successors.size(); ++s) {
                const int target = f.blocks[b].successors[s];
                if (target < 0 || static_cast<std::size_t>(target) >= count) {
                    error = "invalid successor"; return false;
                }
                if (!reachable[target]) { reachable[target] = true; changed = true; }
            }
    }
    Initialized universe, parameters;
    for (Values::const_iterator it = values.begin(); it != values.end(); ++it) universe.insert(it->first);
    for (std::size_t p = 0; p < f.parameterNames.size(); ++p) parameters.insert(f.parameterNames[p]);
    std::vector<Initialized> incoming(count, universe), outgoing(count, universe);
    changed = true;
    while (changed) {
        changed = false;
        for (std::size_t b = 0; b < count; ++b) if (reachable[b]) {
            Initialized state = b == 0 ? parameters : universe;
            if (b != 0) for (std::size_t p = 0; p < count; ++p) if (reachable[p])
                for (std::size_t s = 0; s < f.blocks[p].successors.size(); ++s)
                    if (f.blocks[p].successors[s] == static_cast<int>(b)) {
                        Initialized intersection;
                        for (Initialized::const_iterator it = state.begin(); it != state.end(); ++it)
                            if (outgoing[p].count(*it)) intersection.insert(*it);
                        state.swap(intersection);
                    }
            incoming[b] = state;
            transfer(f.blocks[b], state);
            if (state != outgoing[b]) { outgoing[b] = state; changed = true; }
        }
    }
    for (std::size_t b = 0; b < count; ++b) if (reachable[b]) {
        Initialized state = incoming[b];
        for (std::size_t i = 0; i < f.blocks[b].instructions.size(); ++i) {
            const IRInstruction& in = f.blocks[b].instructions[i];
            std::vector<std::string> reads;
            if (in.opcode.find("call ") == 0) reads = split(in.operand1);
            else if (in.opcode.find("binary ") == 0) { reads.push_back(in.operand1); reads.push_back(in.operand2); }
            else if (in.opcode == "return" || in.opcode == "branch" ||
                     in.opcode == "initialize" || in.opcode == "constant" ||
                     in.opcode == "assign :" || in.opcode == "assign =") reads.push_back(in.operand1);
            for (std::size_t r = 0; r < reads.size(); ++r)
                if (values.count(reads[r]) && !state.count(reads[r])) {
                    std::ostringstream detail;
                    detail << "line " << in.sourceLine << ": '" << reads[r]
                           << "' may be read before initialization in " << f.name;
                    error = detail.str(); return false;
                }
            if (!in.result.empty()) {
                if (in.opcode.find("declare ") == 0) state.erase(in.result);
                else state.insert(in.result);
            }
        }
    }
    return true;
}
bool function(const IRProgram& p, const IRFunction& f, Tms6747Emitter& e, int& labels, std::string& error) {
    if (f.signature.inputs.size() > 10 || f.signature.outputs.size() > 1 ||
        f.parameterNames.size() != f.signature.inputs.size() ||
        (!f.signature.outputs.empty() && !scalar(f.signature.outputs[0]))) {
        error = "unsupported scalar signature in " + f.name; return false;
    }
    Values values; int slots = 0, arguments = 0;
    for (std::size_t a = 0; a < f.signature.inputs.size(); ++a) {
        if (!scalar(f.signature.inputs[a])) { error = "array/reference/char parameter unsupported"; return false; }
        values[f.parameterNames[a]] = Value(slots++, f.signature.inputs[a].name == "real" ? Real : Int);
    }
    for (std::size_t b = 0; b < f.blocks.size(); ++b)
        for (std::size_t i = 0; i < f.blocks[b].instructions.size(); ++i) {
            const IRInstruction& in = f.blocks[b].instructions[i]; Slot type = Int;
            if (in.opcode == "declare int" || in.opcode == "declare real") {
                if (!in.operand1.empty()) { error = "arrays unsupported"; return false; }
                type = in.opcode == "declare real" ? Real : Int;
            } else if (in.opcode.find("call ") == 0) {
                const IRFunction* callee = find(p, in.opcode.substr(5));
                std::vector<std::string> args = split(in.operand1);
                if (!callee || callee->signature.outputs.size() > 1 || args.size() != callee->signature.inputs.size() || args.size() > 10) {
                    error = "unsupported call " + in.opcode; return false;
                }
                if (static_cast<int>(args.size()) > arguments) arguments = static_cast<int>(args.size());
                if (!callee->signature.outputs.empty()) type = callee->signature.outputs[0].name == "real" ? Real : Int;
            } else if (in.opcode.find("binary ") == 0) {
                Slot a, c;
                if (!kind(in.operand1, values, a) || !kind(in.operand2, values, c)) { error = "unresolved binary operand"; return false; }
                std::string op = in.opcode.substr(7);
                if (op == "+" || op == "-" || op == "*") type = a == Real || c == Real ? Real : Int;
            } else if (in.opcode == "constant" || in.opcode == "initialize" || in.opcode == "assign :" || in.opcode == "assign =") {
                if (!kind(in.operand1, values, type)) { error = "unresolved assignment"; return false; }
            } else if (in.opcode != "return" && in.opcode != "jump" && in.opcode != "branch") {
                error = "unsupported IR operation " + in.opcode + " in " + f.name; return false;
            }
            if (!in.result.empty() && values.find(in.result) == values.end()) values[in.result] = Value(slots++, type);
        }
    if (!initializedReads(f, values, error)) return false;
    int argBase = slots; slots += arguments;
    int blockBase = labels; labels += static_cast<int>(f.blocks.size()) + 1; int exit = labels - 1;
    e.beginFunction(f.name);
    for (std::size_t a = 0; a < f.signature.inputs.size(); ++a)
        e.spillArgument(values[f.parameterNames[a]].type, static_cast<int>(a), values[f.parameterNames[a]].slot);
    for (Values::const_iterator it = values.begin(); it != values.end(); ++it) {
        if (it->first.empty() || it->first[0] == '%') continue;
        bool param = false;
        for (std::size_t a = 0; a < f.parameterNames.size(); ++a) if (f.parameterNames[a] == it->first) param = true;
        e.debugVariable(it->first, it->second.slot, slots, param, it->second.type == Real);
    }
    for (std::size_t b = 0; b < f.blocks.size(); ++b) {
        e.label(blockBase + static_cast<int>(b));
        for (std::size_t i = 0; i < f.blocks[b].instructions.size(); ++i) {
            const IRInstruction& in = f.blocks[b].instructions[i]; e.sourceLine(in.sourceLine);
            if (in.opcode.find("declare ") == 0) continue;
            if (in.opcode == "constant" || in.opcode == "initialize" || in.opcode == "assign :" || in.opcode == "assign =") {
                if (!load(e, in.operand1, values, values[in.result].type)) { error = "unsupported assignment"; return false; }
                e.storeSlot(values[in.result].type, values[in.result].slot);
            } else if (in.opcode.find("binary ") == 0) {
                Slot a, c; kind(in.operand1, values, a); kind(in.operand2, values, c);
                Slot type = a == Real || c == Real ? Real : Int;
                std::string op = in.opcode.substr(7), instruction;
                if (!load(e, in.operand1, values, type)) return false;
                e.setArg(type, 1);
                if (!load(e, in.operand2, values, type)) return false;
                if (type == Int) {
                    if (op == "+") instruction = "ADD B4, A4, A4";
                    else if (op == "-") instruction = "SUB B4, A4, A4";
                    else if (op == "*") instruction = "MPY32 B4, A4, A4";
                    else if (op == ">" || op == "<=") instruction = "CMPGT B4, A4, A4";
                    else if (op == "<" || op == ">=") instruction = "CMPLT B4, A4, A4";
                    else if (op == "=" || op == "==" || op == "!=") instruction = "CMPEQ B4, A4, A4";
                } else {
                    if (op == "+") instruction = "ADDDP B5:B4, A5:A4, A5:A4";
                    else if (op == "-") {
                        // ASM6x requires SUBDP's first pair on the destination side.
                        // A6:A7 are volatile scratch; parameters already live in slots.
                        e.nativeInstruction("MV B4, A6"); e.nativeInstruction("MV B5, A7");
                        instruction = "SUBDP A7:A6, A5:A4, A5:A4";
                    }
                    else if (op == "*") instruction = "MPYDP B5:B4, A5:A4, A5:A4";
                }
                if (instruction.empty()) { error = "unsupported binary " + op; return false; }
                e.nativeInstruction(instruction); e.nativeInstruction("NOP 9");
                if (op == ">=" || op == "<=" || op == "!=") e.nativeInstruction("XOR 1, A4, A4");
                e.storeSlot(values[in.result].type, values[in.result].slot);
            } else if (in.opcode.find("call ") == 0) {
                const IRFunction* callee = find(p, in.opcode.substr(5)); std::vector<std::string> args = split(in.operand1);
                for (std::size_t a = 0; a < args.size(); ++a) {
                    if (!scalar(callee->signature.inputs[a])) { error = "unsupported call parameter"; return false; }
                    Slot type = callee->signature.inputs[a].name == "real" ? Real : Int;
                    if (!load(e, args[a], values, type)) { error = "unsupported call operand"; return false; }
                    e.storeSlot(type, argBase + static_cast<int>(a));
                }
                for (std::size_t a = 0; a < args.size(); ++a)
                    e.loadSlotIntoArg(callee->signature.inputs[a].name == "real" ? Real : Int, argBase + static_cast<int>(a), static_cast<int>(a));
                e.call(callee->name);
                if (!in.result.empty()) e.storeSlot(values[in.result].type, values[in.result].slot);
            } else if (in.opcode == "return") {
                if (!f.signature.outputs.empty() && !load(e, in.operand1, values, f.signature.outputs[0].name == "real" ? Real : Int)) {
                    error = "unsupported return"; return false;
                }
                e.jump(exit);
            } else if (in.opcode == "jump") {
                if (f.blocks[b].successors.size() != 1) { error = "malformed jump"; return false; }
                e.jump(blockBase + f.blocks[b].successors[0]);
            } else if (in.opcode == "branch") {
                if (f.blocks[b].successors.size() != 2 || !load(e, in.operand1, values, Int)) { error = "unsupported branch"; return false; }
                e.jumpIfZero(blockBase + f.blocks[b].successors[1]); e.jump(blockBase + f.blocks[b].successors[0]);
            }
        }
    }
    e.label(exit); e.endFunction(slots); return true;
}
}
bool IRAssembly::emit(const IRProgram& program, std::string& assembly, std::string& error, bool symbols) const {
    if (!program.globals.empty()) { error = "TMS6747: global initialization unsupported"; return false; }
    Tms6747Emitter emitter; emitter.configureNativeDebug(symbols, program.sourceFile);
    emitter.beginModule(program.sourceFile); int labels = 0;
    for (std::size_t i = 0; i < program.functions.size(); ++i)
        if (!function(program, program.functions[i], emitter, labels, error)) { error = "TMS6747: " + error; return false; }
    emitter.endModule(); assembly = emitter.text(); return true;
}
}}
