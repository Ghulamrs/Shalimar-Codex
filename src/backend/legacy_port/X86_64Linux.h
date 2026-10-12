
#pragma once

#include "X86_64.h"

namespace shalimar {

class X86_64LinuxEmitter : public X86_64Emitter {
public:
    X86_64LinuxEmitter() : X86_64Emitter(spelling_impl_, systemVAbi()) {}

    std::string symbol(const std::string &name) const  { return name; }

    void beginModule(const std::string &sourceName) ;
    void endModule() ;
    void beginFunction(const std::string &name) ;
    void endFunction(int slots) ;
    void label(int id) ;

private:

    std::string labelName(int id) const  { return ".Lshm" + legacyToString(id); }
    std::string bytesLabel(int id) const  { return ".Lshmb" + legacyToString(id); }
    void openConstSection() ;
    void closeConstSection() ;
    void emitGlobalBlock(int slots) ;
    std::string globalsLabel() const  { return "shm_globals"; }

    GnuSpelling spelling_impl_;
};

}
