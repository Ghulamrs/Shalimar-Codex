#include "LegacyCompat.h"
#include <cstdlib>
#include "Tms6747.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace shalimar {

static std::string debugQuote(const std::string& value) {
    std::string result;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '"' || value[i] == '\\') result += '\\';
        result += value[i];
    }
    return result;
}
void Tms6747Emitter::configureNativeDebug(bool dwarf, const std::string& source) {
    exceptionIndex_ = false; // Shalimar has no C++ exceptions; use DWARF frames for debugging.
    dwarf_ = dwarf;
    debugSource_ = debugQuote(source);
}
void Tms6747Emitter::sourceLine(int line) {
    if (dwarf_ && line > 0) {
        if (!firstDebugLine_) {
            firstDebugLine_ = line;
            const std::string tag = "$SHM_FN_" + currentFunction_;
            raw("\t.dwattr " + tag + ", DW_AT_TI_begin_file(\"" + debugSource_ + "\")");
            raw("\t.dwattr " + tag + ", DW_AT_TI_begin_line(" + legacyToString(line) + ")");
            raw("\t.dwattr " + tag + ", DW_AT_TI_begin_column(1)");
            raw("\t.dwattr " + tag + ", DW_AT_decl_file(\"" + debugSource_ + "\")");
            raw("\t.dwattr " + tag + ", DW_AT_decl_line(" + legacyToString(line) + ")");
            raw("\t.dwattr " + tag + ", DW_AT_decl_column(1)");
            raw("\t.dwpsn file \"" + debugSource_ + "\",line " + legacyToString(line) + ",column 1,is_stmt,address " + currentFunction_ + ",isa 0");
        }
        if (line > lastDebugLine_) lastDebugLine_ = line;
        raw("\t.dwpsn file \"" + debugSource_ + "\",line " + legacyToString(line) + ",column 1,is_stmt,isa 0");
    }
}
void Tms6747Emitter::debugVariable(const std::string& name, int slot, int slots, bool parameter, bool real) {
    if (!dwarf_) return;
    const std::string tag = "$SHM_VAR_" + currentFunction_ + "_" + legacyToString(slot);
    raw(tag + "\t.dwtag " + (parameter ? "DW_TAG_formal_parameter" : "DW_TAG_variable") + ", DW_AT_name(\"" + debugQuote(name) + "\")");
    raw("\t.dwattr " + tag + ", DW_AT_type(*" + (real ? "$SHM_REAL" : "$SHM_INT") + ")");
    raw("\t.dwattr " + tag + ", DW_AT_location[DW_OP_breg15 " + legacyToString(8 * slot - (40 + 8 * slots)) + "]");
}

static const int kSaveBytes = 40;   // under A15 whatever is saved, so a slot's address does not wait on the body

void Tms6747Emitter::beginModule(const std::string &sourceName) {
    raw("; " + sourceName);
    if (dwarf_) raw("\t.compiler_opts --abi=eabi --endian=little --hll_source=on --object_format=elf --silicon_version=6740 --symdebug:dwarf --symdebug:dwarf_version=3");
    raw("\t.text");
    if (dwarf_) {
        raw("$SHM_CU\t.dwtag DW_TAG_compile_unit");
        raw("\t.dwattr $SHM_CU, DW_AT_name(\"" + debugSource_ + "\")");
        raw("\t.dwattr $SHM_CU, DW_AT_TI_version(1)");
        const std::size_t slash = sourceName.find_last_of("/\\");
        if (slash != std::string::npos)
            raw("\t.dwattr $SHM_CU, DW_AT_comp_dir(\"" + debugQuote(sourceName.substr(0, slash)) + "\")");
        raw("\t.dwattr $SHM_CU, DW_AT_producer(\"Shalimar TMS6747 Rev3\")");
        raw("\t.dwattr $SHM_CU, DW_AT_language(DW_LANG_C)");
        raw("$SHM_INT\t.dwtag DW_TAG_base_type");
        raw("\t.dwattr $SHM_INT, DW_AT_name(\"int\")");
        raw("\t.dwattr $SHM_INT, DW_AT_encoding(DW_ATE_signed)");
        raw("\t.dwattr $SHM_INT, DW_AT_byte_size(4)");
        raw("$SHM_REAL\t.dwtag DW_TAG_base_type");
        raw("\t.dwattr $SHM_REAL, DW_AT_name(\"real\")");
        raw("\t.dwattr $SHM_REAL, DW_AT_encoding(DW_ATE_float)");
        raw("\t.dwattr $SHM_REAL, DW_AT_byte_size(8)");
    }
}

void Tms6747Emitter::endModule() {
    if (globalSlots_ > 0) {
        blank();
        raw("\t.global\t" + symbol("shm_globals"));
        raw("\t.bss\t" + symbol("shm_globals") + ", " +
            legacyToString(globalSlots_ * 8) + ", 8");
    }
    if (!data_.empty()) {
        blank();
        raw("\t.sect\t\".const\"");
        text_ += data_;
    }
    for (std::set<std::string>::const_iterator it = called_.begin(); it != called_.end(); ++it)
        if (!defined_.count(*it)) raw("\t.ref\t" + symbol(*it));
    // The personality routine the index entries name by number.
    if (exceptionIndex_ && !currentFunction_.empty()) {
        raw("\t.global\t__c6xabi_unwind_cpp_pr3");
        raw("\t.symdepend\t\"__c6xabi_unwind_cpp_pr3\", \".c6xabi.exidx:.text\"");
    }
    if (dwarf_) {
        raw("$SHM_CIE\t.dwcie 19");
        raw("\t.dwcfi cfa_register, 31");
        raw("\t.dwcfi cfa_offset, 0");
        raw("\t.dwcfi same_value, 15");
        raw("\t.dwcfi same_value, 19");
        raw("\t.dwcfi same_value, 31");
        raw("\t.dwendentry");
        raw("\t.dwendtag $SHM_CU");
    }
    blank();
}

void Tms6747Emitter::constant(const std::string &reg, int32_t value) {
    instruction("MVKL\t" + legacyToString(value) + ", " + reg);
    instruction("MVKH\t" + legacyToString(value) + ", " + reg);
}

// The slots rise from the bottom of the frame, so a block of them reads as an array; how far
// below A15 that is - the save area and every slot - is known only when the function ends, and
// is a symbol the assembler resolves: slot k is at A15 - base + 8k.
void Tms6747Emitter::slotAddress(int slot, const std::string &reg) {
    const std::string off = legacyToString(8 * slot) + " - " + slotBase_;
    instruction("MVKL\t" + off + ", " + reg);
    instruction("MVKH\t" + off + ", " + reg);
    instruction("ADD\tA15, " + reg + ", " + reg);
}

std::string Tms6747Emitter::pairOf(const std::string &reg) {
    // "A4" -> "A5:A4"
    const int n = atoi(reg.c_str() + 1);
    return reg.substr(0, 1) + legacyToString(n + 1) + ":" + reg;
}

std::string Tms6747Emitter::argRegister(int index) {
    const int number = 4 + 2 * (index / 2);
    return std::string(index % 2 == 0 ? "A" : "B") + legacyToString(number);
}

void Tms6747Emitter::loadAt(Slot kind, const std::string &addrReg, const std::string &reg) {
    if (kind == Int) instruction("LDW\t*" + addrReg + ", " + reg);
    else                   instruction("LDDW\t*" + addrReg + ", " + pairOf(reg));
    instruction("NOP\t4");
}

void Tms6747Emitter::storeAt(Slot kind, const std::string &addrReg, const std::string &reg) {
    if (kind == Int) instruction("STW\t" + reg + ", *" + addrReg);
    else                   instruction("STDW\t" + pairOf(reg) + ", *" + addrReg);
}

std::string Tms6747Emitter::labelName(int id) { return "Lshm" + legacyToString(id); }
std::string Tms6747Emitter::bytesLabel(int id) { return "Lshmb" + legacyToString(id); }

void Tms6747Emitter::beginFunction(const std::string &name) {
    firstDebugLine_ = lastDebugLine_ = 0;
    blank();
    raw("\t.global\t" + symbol(name));
    if (dwarf_) {
        const std::string tag = "$SHM_FN_" + symbol(name);
        raw(tag + "\t.dwtag DW_TAG_subprogram, DW_AT_name(\"" + debugQuote(name) + "\")");
        raw("\t.dwattr " + tag + ", DW_AT_low_pc(" + symbol(name) + ")");
        raw("\t.dwattr " + tag + ", DW_AT_high_pc(0)");
        raw("\t.dwattr " + tag + ", DW_AT_TI_symbol_name(\"" + symbol(name) + "\")");
        raw("\t.dwattr " + tag + ", DW_AT_external");
        raw("\t.dwfde $SHM_CIE, " + symbol(name));
    }
    raw(symbol(name) + ":");
    defined_.insert(name);
    currentFunction_ = symbol(name);
    slotBase_ = "shmbase$" + symbol(name);
    usesSavedArgRegs_ = false;
    usesSavedPairRegs_ = false;
    outgoingBytes_ = 0;
    incomingBytes_ = 0;
    prologueMark_ = text_.size();
}

// The frame, the shape c90 and cpp11 keep: A15 points at the caller's B15 word and holds the caller's A15; below it, in
// the order TI's unwinder pops them, B12, B10, B3, A12, A10 when the body loads the argument registers and B3 alone
// otherwise, in 40 bytes whatever is saved; then the slots, eight bytes each; then the outgoing arguments past the registers, from B15 + 4, the word at B15 the callee's.

static std::vector<std::string> savedRegs(bool argRegs, bool pairRegs) {
    std::vector<std::string> r;
    r.push_back("A15");
    if (pairRegs) r.push_back("B13");
    if (argRegs) r.push_back("B12");
    if (pairRegs) r.push_back("B11");
    if (argRegs) r.push_back("B10");
    r.push_back("B3");
    if (pairRegs) r.push_back("A13");
    if (argRegs) r.push_back("A12");
    if (pairRegs) r.push_back("A11");
    if (argRegs) r.push_back("A10");
    return r;
}
void Tms6747Emitter::endFunction(int slots) {
    const std::vector<std::string> saved = savedRegs(usesSavedArgRegs_, usesSavedPairRegs_);
    const int outgoing = (4 + outgoingBytes_ + 7) / 8 * 8;   // the callee's word, then the arguments
    const int frame = kSaveBytes + 8 * slots + outgoing;
    std::string prologue;
    prologue += slotBase_ + "\t.set\t" + legacyToString(kSaveBytes + 8 * slots) + "\n";   // before its uses
    prologue += "\tSTW\tA15, *B15\n";
    prologue += "\tMV\tB15, A15\n";
    if (dwarf_) prologue += "\t.dwcfi save_reg_to_mem, 15, 0\n";
    for (size_t k = 1; k < saved.size(); k++) {
        prologue += "\tSTW\t" + saved[k] + ", *-A15(" + legacyToString(4 * k) + ")\n";
        if (dwarf_) {
            const int reg = atoi(saved[k].c_str() + 1) + (saved[k][0] == 'B' ? 16 : 0);
            prologue += "\t.dwcfi save_reg_to_mem, " + legacyToString(reg) + ", " + legacyToString(-4 * static_cast<int>(k)) + "\n";
        }
    }
    prologue += "\tMVKL\t" + legacyToString(frame) + ", A3\n";
    prologue += "\tMVKH\t" + legacyToString(frame) + ", A3\n";
    prologue += "\tSUB\tB15, A3, B15\n";
    if (dwarf_) prologue += "\t.dwcfi cfa_offset, " + legacyToString(frame) + "\n";
    text_.insert(prologueMark_, prologue);

    for (size_t k = 1; k < saved.size(); k++)
        instruction("LDW\t*-A15(" + legacyToString(4 * k) + "), " + saved[k]);
    instruction("MV\tA15, B15");
    instruction("LDW\t*A15, A15");
    instruction("NOP\t4");
    if (dwarf_) {
        raw("\t.dwcfi cfa_register, 31");
        raw("\t.dwcfi cfa_offset, 0");
        raw("\t.dwcfi restore_reg, 15");
        for (size_t k = 1; k < saved.size(); k++) {
            const int reg = atoi(saved[k].c_str() + 1) + (saved[k][0] == 'B' ? 16 : 0);
            raw("\t.dwcfi restore_reg, " + legacyToString(reg));
        }
    }
    instruction("B\tB3");
    instruction("NOP\t5");
    if (exceptionIndex_) indexEntry(usesSavedArgRegs_, usesSavedPairRegs_);
    if (dwarf_) {
        raw("\t.dwattr $SHM_FN_" + currentFunction_ + ", DW_AT_TI_end_file(\"" + debugSource_ + "\")");
        raw("\t.dwattr $SHM_FN_" + currentFunction_ + ", DW_AT_TI_end_line(" + legacyToString(lastDebugLine_) + ")");
        raw("\t.dwattr $SHM_FN_" + currentFunction_ + ", DW_AT_TI_end_column(1)");
        raw("\t.dwendentry");
        raw("\t.dwendtag $SHM_FN_" + currentFunction_);
    }
}

// TI's exception index entry for the function just ended, in the compact
// form (lib/src/tdeh_pr_c6000.cpp): personality 3, SP restored from A15
// (0x7f), the bitmask of the registers saved, B3 the return register.
void Tms6747Emitter::indexEntry(bool argRegs, bool pairRegs) {
    static const struct { const char *reg; int bit; } bits[] = {
        { "A15", 12 }, { "B13", 9 }, { "B12", 8 }, { "B11", 7 }, { "B10", 6 }, { "B3", 5 },
        { "A13", 3 }, { "A12", 2 }, { "A11", 1 }, { "A10", 0 } };
    unsigned mask = 0;
    const std::vector<std::string> saved = savedRegs(argRegs, pairRegs);
    for (std::vector<std::string>::const_iterator it = saved.begin(); it != saved.end(); ++it)
        for (size_t k = 0; k < sizeof bits / sizeof bits[0]; k++)
            if (*it == bits[k].reg) mask |= 1u << bits[k].bit;
    char word[16];
    std::snprintf(word, sizeof word, "0x%08x", 0x83000000u | 0x7fu << 17 | mask << 4 | 7u);
    raw("\t.sect\t\".c6xabi.exidx:.text\"");
    raw("\t.align\t4");
    raw("\t.ulong\t$EXIDX_FUNC(" + currentFunction_ + ")");
    raw("\t.ulong\t" + std::string(word));
    raw("\t.text");
}

void Tms6747Emitter::loadIntConstant(int32_t value) {
    constant("A4", value);
}

void Tms6747Emitter::loadRealConstant(double value) {
    const uint64_t bits = bitsOf(value);
    constant("A4", static_cast<int32_t>(static_cast<uint32_t>(bits & 0xFFFFFFFFu)));
    constant("A5", static_cast<int32_t>(static_cast<uint32_t>(bits >> 32)));
}

void Tms6747Emitter::storeSlot(Slot kind, int slot) {
    slotAddress(slot, "A3");
    storeAt(kind, "A3", "A4");
}

void Tms6747Emitter::loadSlot(Slot kind, int slot) {
    slotAddress(slot, "A3");
    loadAt(kind, "A3", "A4");
}

void Tms6747Emitter::setArg(Slot kind, int index) {
    if (index == 0) return;
    if (index >= 6) usesSavedArgRegs_ = true;
    if (index >= 6 && kind != Int) usesSavedPairRegs_ = true;   // the pair's partner is callee-saved too
    const std::string reg = argRegister(index);
    instruction("MV\tA4, " + reg);
    if (kind != Int) instruction("MV\tA5, " + pairOf(reg).substr(0, pairOf(reg).find(':')));
}

void Tms6747Emitter::loadSlotIntoArg(Slot kind, int slot, int index) {
    if (index >= 6) usesSavedArgRegs_ = true;
    if (index >= 6 && kind != Int) usesSavedPairRegs_ = true;
    slotAddress(slot, "A3");
    loadAt(kind, "A3", argRegister(index));
}

void Tms6747Emitter::spillArgument(Slot kind, int registerIndex, int slot) {
    slotAddress(slot, "A3");
    storeAt(kind, "A3", argRegister(registerIndex));
}

// The arguments past the ten registers go where the ABI puts them: from
// the caller's B15 + 4 upward, a word for an int or a pointer, a real in
// eight bytes at an 8-aligned offset. Both sides count the same way.
static int argumentOffset(int &bytes, Slot kind) {
    int at = 4 + bytes;
    if (kind == Real) at = (at + 7) / 8 * 8;     // the offset from B15 is what aligns
    bytes = at - 4 + (kind == Real ? 8 : 4);
    return at;
}
// Through A0-A3 only: the argument registers are already loaded.
void Tms6747Emitter::setOverflowBlock(int slot, const std::vector<Slot> &kinds) {
    int bytes = 0;
    for (size_t i = 0; i < kinds.size(); i++) {
        const int at = argumentOffset(bytes, kinds[i]);
        slotAddress(slot + static_cast<int>(i), "A3");
        loadAt(kinds[i], "A3", "A0");
        constant("A2", at);
        instruction("ADD\tB15, A2, A2");
        if (kinds[i] == Real) instruction("STDW\tA1:A0, *A2");
        else                        instruction("STW\tA0, *A2");
    }
    if (bytes > outgoingBytes_) outgoingBytes_ = bytes;
}

void Tms6747Emitter::spillOverflowArgument(Slot kind, int index, int slot) {
    (void)index;                                   // they arrive in order
    const int at = argumentOffset(incomingBytes_, kind);
    constant("A6", at);
    instruction("ADD\tA15, A6, A6");
    if (kind == Real) instruction("LDDW\t*A6, A9:A8");
    else                    instruction("LDW\t*A6, A8");
    instruction("NOP\t4");
    if (kind == Wide) instruction("ZERO\tA9");
    slotAddress(slot, "A3");
    storeAt(kind, "A3", "A8");
}

void Tms6747Emitter::call(const std::string &name) {
    const std::string back = "Lshmr" + legacyToString(returns_++);
    instruction("MVKL\t" + back + ", B3");
    instruction("MVKH\t" + back + ", B3");
    instruction("B\t" + symbol(name));
    instruction("NOP\t5");
    raw(back + ":");
    called_.insert(name);
}

void Tms6747Emitter::loadSlotAddress(int slot) {
    slotAddress(slot, "A4");
}

void Tms6747Emitter::storeThroughPointer(Slot kind, int pointerSlot) {
    slotAddress(pointerSlot, "A3");
    instruction("LDW\t*A3, A3");
    instruction("NOP\t4");
    storeAt(kind, "A3", "A4");
}

void Tms6747Emitter::loadThroughPointer(Slot kind, int pointerSlot) {
    slotAddress(pointerSlot, "A3");
    instruction("LDW\t*A3, A3");
    instruction("NOP\t4");
    loadAt(kind, "A3", "A4");
}

void Tms6747Emitter::defineGlobals(int slots) {
    globalSlots_ = slots;
}

void Tms6747Emitter::loadGlobal(Slot kind, int index) {
    instruction("MVKL\t" + symbol("shm_globals") + ", A3");
    instruction("MVKH\t" + symbol("shm_globals") + ", A3");
    constant("A6", 8 * index);
    instruction("ADD\tA3, A6, A3");
    loadAt(kind, "A3", "A4");
}

void Tms6747Emitter::storeGlobal(Slot kind, int index) {
    instruction("MVKL\t" + symbol("shm_globals") + ", A3");
    instruction("MVKH\t" + symbol("shm_globals") + ", A3");
    constant("A6", 8 * index);
    instruction("ADD\tA3, A6, A3");
    storeAt(kind, "A3", "A4");
}

void Tms6747Emitter::defineBytes(int id, const std::string &bytes) {
    data_ += bytesLabel(id) + ":\n";
    std::string line;
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (line.empty()) line = "\t.byte\t";
        else line += ", ";
        line += legacyToString(static_cast<unsigned char>(bytes[i]));
        if (i % 16 == 15) { data_ += line + "\n"; line.clear(); }
    }
    if (!line.empty()) data_ += line + "\n";
}

void Tms6747Emitter::loadBytesAddress(int id) {
    instruction("MVKL\t" + bytesLabel(id) + ", A4");
    instruction("MVKH\t" + bytesLabel(id) + ", A4");
}

// An int made wide: the sign copied into the high word of the pair.
void Tms6747Emitter::widenAccumulator() {
    instruction("SHR\tA4, 31, A5");
}

void Tms6747Emitter::label(int id) {
    raw(labelName(id) + ":");
}

void Tms6747Emitter::jump(int id) {
    instruction("B\t" + labelName(id));
    instruction("NOP\t5");
}

void Tms6747Emitter::jumpIfZero(int id) {
    instruction("MV\tA4, A1");
    instruction("[!A1]\tB\t" + labelName(id));
    instruction("NOP\t5");
}

}
