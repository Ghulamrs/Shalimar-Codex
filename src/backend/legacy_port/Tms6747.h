#pragma once

#include "Emitter.h"
#include "LegacyCompat.h"

#include <set>

namespace shalimar {

// The TMS320C6747 (C6000) as the Shalimar compiler's fourth target: C6000 assembly text, serial,
// every delay slot a NOP - the conventions c90 and cpp11 emit for it, which is what lets one
// emulator run all three and the runtime sit beside a Shalimar program. The register conventions are in VM6747/TMS6747.md, "The shalimar backend's header".
class Tms6747Emitter : public Emitter {
public:
    Tms6747Emitter() : prologueMark_(0), returns_(0), globalSlots_(0), usesSavedArgRegs_(false), usesSavedPairRegs_(false), outgoingBytes_(0), incomingBytes_(0), exceptionIndex_(true), dwarf_(false) {}
    void configureNativeDebug(bool dwarf, const std::string& source);
    void sourceLine(int line);
    void debugVariable(const std::string& name, int slot, int slots, bool parameter, bool real);
    void nativeInstruction(const std::string& value) { instruction(value); }
    void nativeRaw(const std::string& value) { raw(value); }
    // A borrowed C99 name as TI's runtime spells it; the emulator answers to both.
    std::string symbol(const std::string &name) const  {
        if (name == "trunc") return "__c6xabi_trunc";
        if (name == "round") return "__c6xabi_nround";
        return name;
    }

    void beginModule(const std::string &sourceName) ;
    void endModule() ;

    void beginFunction(const std::string &name) ;
    void endFunction(int slots) ;

    void loadIntConstant(int32_t value) ;
    void loadRealConstant(double value) ;

    void storeSlot(Slot kind, int slot) ;
    void loadSlot(Slot kind, int slot) ;
    void setArg(Slot kind, int index) ;
    void loadSlotIntoArg(Slot kind, int slot, int index) ;

    bool positionalArguments() const  { return true; }
    int intArgCapacity() const  { return 10; }
    int realArgCapacity() const  { return 10; }
    void setOverflowBlock(int slot, const std::vector<Slot> &kinds) ;
    void spillOverflowArgument(Slot kind, int index, int slot) ;
    void spillArgument(Slot kind, int registerIndex, int slot) ;
    void call(const std::string &name) ;
    void widenAccumulator() ;

    void loadSlotAddress(int slot) ;
    void storeThroughPointer(Slot kind, int pointerSlot) ;
    void loadThroughPointer(Slot kind, int pointerSlot) ;

    void defineGlobals(int slots) ;
    void loadGlobal(Slot kind, int index) ;
    void storeGlobal(Slot kind, int index) ;

    void defineBytes(int id, const std::string &bytes) ;
    void loadBytesAddress(int id) ;

    void label(int id) ;
    void jump(int id) ;
    void jumpIfZero(int id) ;

private:
    size_t prologueMark_;
    int returns_;
    int globalSlots_;
    // The TI assembler wants an undefined name declared: what was called, less what this file defines, is `.ref`ed when the module ends.
    std::set<std::string> called_, defined_;
    // A10, B10, A12 and B12 carry arguments seven to ten and are the
    // callee's to keep: a function that loads them saves them in its frame.
    bool usesSavedArgRegs_;
    bool usesSavedPairRegs_;   // A11/B11/A13/B13, which a real in A10-B12 writes
    std::string currentFunction_;
    std::string slotBase_;       // the symbol for this function's slot 0, below A15
    int outgoingBytes_;      // the widest set of arguments this function passes on the stack
    int incomingBytes_;      // the arguments it has taken from its own caller's, so far
    bool exceptionIndex_;
    bool dwarf_;
    std::string debugSource_;
    int firstDebugLine_;
    int lastDebugLine_;
    void indexEntry(bool argRegs, bool pairRegs);

    // A constant into a register, MVKL then MVKH - the only way to a 32-bit value.
    void constant(const std::string &reg, int32_t value);
    // The address of a slot into `reg`: A15 - base + 8*slot.
    void slotAddress(int slot, const std::string &reg);
    // A load or store of `kind` at *reg, into or from the accumulator or a given register.
    void loadAt(Slot kind, const std::string &addrReg, const std::string &reg);
    void storeAt(Slot kind, const std::string &addrReg, const std::string &reg);
    // The register a positional argument rides in: A4, B4, A6, ...
    static std::string argRegister(int index);
    static std::string pairOf(const std::string &reg);

    static std::string labelName(int id);
    static std::string bytesLabel(int id);
};

}
