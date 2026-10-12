
#pragma once

#include "Emitter.h"

namespace shalimar {

class Arm64DarwinEmitter : public Emitter {
public:
    Arm64DarwinEmitter() : prologueMark_(0), frameBytes_(0) {}
    std::string symbol(const std::string &name) const  { return "_" + name; }

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

    bool positionalArguments() const  { return false; }
    int intArgCapacity() const  { return 8; }
    int realArgCapacity() const  { return 8; }
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

    static const int frameRecordBytes = 16;

    size_t prologueMark_;
    int frameBytes_;

    std::string slotAddress(int slot, int width);

    void adjustStack(const char *op, int bytes);

    void scratchConstant(uint64_t value);

    static char registerFile(Slot kind);
    static int byteWidth(Slot kind);
    std::string globalAddress(int index, int width);

    static std::string labelName(int id);
    static std::string bytesLabel(int id);

    void addressGlobals();

    void materialise(const std::string &reg, uint64_t bits, bool wide);
};

}
