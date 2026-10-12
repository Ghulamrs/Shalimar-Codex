
#pragma once

#include "Emitter.h"
#include "Ins.h"
#include "Optimize.h"
#include "Spelling.h"

#include <string>
#include <vector>

namespace shalimar {

struct Abi {
    const Reg *intArgs;
    int intArgCount;
    int sseArgCount;

    bool positional;

    int shadowBytes;
};

const Abi &systemVAbi();
const Abi &microsoftAbi();

class X86_64Emitter : public Emitter {
public:
    X86_64Emitter(const Spelling &spelling, const Abi &abi)
        : optimize_(0), spelling_(spelling), abi_(abi) {}

    void loadIntConstant(int32_t value) ;
    void loadRealConstant(double value) ;

    void storeSlot(Slot kind, int slot) ;
    void loadSlot(Slot kind, int slot) ;
    void setArg(Slot kind, int index) ;
    void loadSlotIntoArg(Slot kind, int slot, int index) ;

    bool positionalArguments() const  { return abi_.positional; }
    int intArgCapacity() const  { return abi_.intArgCount; }
    int realArgCapacity() const  { return abi_.sseArgCount; }
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

    void jump(int id) ;
    void jumpIfZero(int id) ;

    void setOptimize(int level)  { optimize_ = level; }

protected:
    // **The structured funnel.** Every x86 instruction goes through here as a
    // record; the spelling turns it into text at the last moment, which is
    // where the optimizer sits.

    // Buffered, not spelled: optimizeRun sees the whole straight-line stretch.
    void emit(const Ins &i) { run_.push_back(i); }

    // **Anything unbuffered writes the buffer out first**, which is what makes
    // a run a stretch with no branch into or out of it.
    void instruction(const std::string &line)  {
        flushRun();
        Emitter::instruction(line);
    }
    void raw(const std::string &line)  {
        flushRun();
        Emitter::raw(line);
    }
    void flushRun() {
        if (run_.empty()) return;
        if (optimize_ >= 1) optimizeRun(run_);
        std::vector<Ins> done;
        done.swap(run_);
        for (std::vector<Ins>::const_iterator it = done.begin(); it != done.end(); ++it) Emitter::instruction(spelling_.render(*it));
    }

    std::vector<Ins> run_;
    int optimize_;

    // The three the spelling names itself, because each syntax writes them
    // its own way: a `lea` whose source is an address, the widening that is
    // `movsxd` on one side and `movslq` on the other, and a call.
    void emitLea(const Operand &from, const Operand &to) {
        instruction(spelling_.loadAddress(spelling_.render(from), spelling_.render(to)));
    }
    void emitWiden(const Operand &from, const Operand &to) {
        instruction(spelling_.widen32To64(spelling_.render(from), spelling_.render(to)));
    }
    void emitCall(const std::string &target) { instruction(spelling_.call(target)); }

    const Spelling &spelling_;
    const Abi &abi_;

    static const Reg accumulator = Ax;
    static const Reg realAccumulator = Xmm0;

    static const Reg overflowPointer = R10;

    virtual std::string labelName(int id) const = 0;
    virtual std::string bytesLabel(int id) const = 0;

    virtual void openConstSection() = 0;
    virtual void closeConstSection() = 0;

    virtual void emitGlobalBlock(int slots) = 0;
    virtual std::string globalsLabel() const = 0;

    Operand slotOperand(int slot, int width) const;

    std::string prologue(int slots);
    void emitEpilogue(int slots);
    int frameBytesFor(int slots) const;

    size_t prologueMark_;

    void noteExternal(const std::string &name) ;
    void noteDefined(const std::string &name);

    std::vector<std::string> externals() const;

private:
    std::vector<std::string> referenced_;
    std::vector<std::string> defined_;

    static Reg registerFor(Slot kind);
    static const char *moveFor(Slot kind);
    static int widthFor(Slot kind);
    static Reg sseArg(int index);
    Reg argRegister(Slot kind, int index) const;
};

}
