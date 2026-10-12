// Native local int/real-vector lowering. Unsupported forms fail explicitly.
struct NativeNumericArray {
    int displacement; unsigned count; bool real;
    NativeNumericArray() : displacement(0), count(0), real(false) {}
    NativeNumericArray(int d, unsigned c, bool r=false) : displacement(d), count(c), real(r) {}
};
static bool arrayLoad(InstructionEncoder& e, const std::string& value,
                      const std::map<std::string,int>& slots, const std::set<std::string>& initialized) {
    unsigned literal = 0;
    if (parseInteger(value, literal)) { e.movEaxImmediate(literal); return true; }
    std::map<std::string,int>::const_iterator found = slots.find(value);
    if (found == slots.end() || !initialized.count(value)) return false;
    e.loadEaxFromRbp(found->second); return true;
}
static bool vectorRealKind(const std::string& name, const std::map<std::string,bool>& kinds) {
    std::map<std::string,bool>::const_iterator it=kinds.find(name);
    if(it!=kinds.end()) return it->second;
    unsigned integer=0; if(parseInteger(name,integer)) return false;
    char* end=0; errno=0; const double value=std::strtod(name.c_str(),&end);
    return !name.empty() && end!=name.c_str() && !*end && !errno && value==value && value<=DBL_MAX && value>=-DBL_MAX;
}
static bool vectorLoadReal(InstructionEncoder& e, const std::string& value,
                           const std::map<std::string,int>& slots, const std::set<std::string>& initialized,
                           const std::map<std::string,bool>& kinds) {
    if(slots.count(value)) {
        if(!initialized.count(value)) return false;
        if(kinds.find(value)!=kinds.end() && kinds.find(value)->second) e.loadXmm0FromRbp(slots.find(value)->second);
        else {e.loadEaxFromRbp(slots.find(value)->second); e.intToReal();}
        return true;
    }
    char* end=0; errno=0; const double number=std::strtod(value.c_str(),&end);
    if(value.empty() || end==value.c_str() || *end || errno || number!=number || number>DBL_MAX || number< -DBL_MAX) return false;
    unsigned words[2]={0,0}; std::memcpy(words,&number,8);
    e.movRaxImmediate64(words[0],words[1]); e.movqXmm0Rax(); return true;
}
static bool selectNumericArrays(const IRFunction& f, InstructionEncoder& e,
                            std::vector<coff::Relocation>& relocations, std::string& error,
                            std::vector<coff::DebugVariable>* debugVariables) {
    if (f.blocks.size() != 1 || f.signature.inputs.size() > 1 || f.signature.outputs.size() > 1 ||
        (!f.signature.inputs.empty() && (f.signature.inputs[0].name != "int" || f.signature.inputs[0].arrayRank || f.signature.inputs[0].byReference)) ||
        (!f.signature.outputs.empty() && ((f.signature.outputs[0].name != "int" && f.signature.outputs[0].name != "real") || f.signature.outputs[0].arrayRank))) {
        error = "native numeric vectors currently require one block, at most one scalar int input, and one int/real output"; return false;
    }
    std::map<std::string,NativeNumericArray> arrays;
    std::map<std::string,int> slots;
    std::set<std::string> declarations;
    unsigned used = 0;
    std::map<std::string,bool> kinds;
    if (!f.parameterNames.empty()) slots[f.parameterNames[0]] = -4 * static_cast<int>(++used);
    const std::vector<IRInstruction>& body = f.blocks[0].instructions;
    for(std::size_t i=0;i<body.size();++i) if(body[i].opcode.find("declare ")==0)
        kinds[body[i].result]=body[i].opcode=="declare real";
    // A literal temporary uses the target array element type, including omitted zeros.
    for(std::size_t i=0;i<body.size();++i) if(body[i].opcode=="initialize" || body[i].opcode.find("assign ")==0)
        if(kinds.count(body[i].result) && !body[i].operand1.empty() && body[i].operand1[0]=='%')
            kinds[body[i].operand1]=kinds[body[i].result];
    for(std::size_t i=0;i<body.size();++i) {
        const IRInstruction& in=body[i];
        if(in.opcode=="index") kinds[in.result]=kinds[in.operand1];
        else if(in.opcode=="unary -") kinds[in.result]=in.operand1!="2147483648" && vectorRealKind(in.operand1,kinds);
        else if(in.opcode.find("binary ")==0) kinds[in.result]=vectorRealKind(in.operand1,kinds)||vectorRealKind(in.operand2,kinds);
        else if(in.opcode=="constant" || in.opcode=="initialize" || in.opcode.find("assign ")==0)
            if(!kinds.count(in.result)) kinds[in.result]=vectorRealKind(in.operand1,kinds);
    }
    for (std::size_t i=0; i<body.size(); ++i) {
        const IRInstruction& in=body[i];
        if (in.opcode.find("declare ")==0 &&
            (!declarations.insert(in.result).second || slots.count(in.result))) {
            error="native vector lowering requires distinct local and parameter names"; return false;
        }
        if (in.opcode == "array.begin" || ((in.opcode == "declare int" || in.opcode=="declare real") && !in.operand1.empty())) {
            unsigned count=0;
            if (!parseInteger(in.operand1,count) || !count || count>512 || arrays.count(in.result) || slots.count(in.result)) {
                error="native local numeric vectors require a single extent from 1 to 512"; return false;
            }
            if(kinds[in.result] && (used%2)) ++used;
            used += count*(kinds[in.result]?2:1); arrays[in.result]=NativeNumericArray(-4*static_cast<int>(used),count,kinds[in.result]);
        } else if (in.opcode.find("declare ")==0 && in.opcode!="declare int" && in.opcode!="declare real") {
            error="native vector function currently supports int/real elements and scalar locals"; return false;
        } else if (!in.result.empty() && !arrays.count(in.result) && !slots.count(in.result)) {
            if(kinds[in.result] && (used%2)) ++used;
            used+=kinds[in.result]?2:1; slots[in.result]=-4*static_cast<int>(used);
        }
    }
    const unsigned frame=(used*4+15)&~15U;
    if (frame>4000) { error="native vector stack frame exceeds current stack-probe limit"; return false; }
    InstructionEncoder candidate;
    candidate.pushRbp(); candidate.movRbpRsp(); candidate.allocateStack(frame);
    std::set<std::string> initialized;
    if (!f.parameterNames.empty()) {
        candidate.movEaxEcx(); candidate.storeEaxAtRbp(slots[f.parameterNames[0]]); initialized.insert(f.parameterNames[0]);
    }
    std::map<std::string,unsigned> arrayLiveStarts;
    if (debugVariables && !f.parameterNames.empty()) {
        coff::DebugVariable parameter;
        parameter.name=f.parameterNames[0]; parameter.typeName="int";
        debugVariables->push_back(parameter);
        arrayLiveStarts[parameter.name]=static_cast<unsigned>(candidate.bytes().size());
    }
    bool returned=false;
    for (std::size_t i=0; i<body.size(); ++i) {
        const IRInstruction& in=body[i];
        candidate.markSource(in.debugLocation.file,static_cast<unsigned>(in.sourceLine),1);
        if (in.opcode=="array.begin" || ((in.opcode=="declare int" || in.opcode=="declare real") && arrays.count(in.result))) {
            const NativeNumericArray& a=arrays[in.result]; candidate.movEaxImmediate(0);
            if(a.real) {candidate.movRaxImmediate64(0,0);candidate.movqXmm0Rax();}
            for (unsigned k=0;k<a.count;++k) {
                if(a.real) candidate.storeXmm0AtRbp(a.displacement+8*static_cast<int>(k));
                else candidate.storeEaxAtRbp(a.displacement+4*static_cast<int>(k));
            }
            arrayLiveStarts[in.result]=static_cast<unsigned>(candidate.bytes().size());
        } else if (in.opcode=="declare int" || in.opcode=="declare real") {
            initialized.erase(in.result);
        } else if (in.opcode=="array.element") {
            unsigned index=0;
            if (!arrays.count(in.result) || !parseInteger(in.operand1,index) || index>=arrays[in.result].count ||
                !(arrays[in.result].real ? vectorLoadReal(candidate,in.operand2,slots,initialized,kinds) :
                (!vectorRealKind(in.operand2,kinds) && arrayLoad(candidate,in.operand2,slots,initialized)))) goto unsupported;
            if(arrays[in.result].real) candidate.storeXmm0AtRbp(arrays[in.result].displacement+8*static_cast<int>(index));
            else candidate.storeEaxAtRbp(arrays[in.result].displacement+4*static_cast<int>(index));
        } else if ((in.opcode=="initialize" || in.opcode=="assign :" || in.opcode=="assign =") && arrays.count(in.result)) {
            if (!arrays.count(in.operand1) || arrays[in.result].count!=arrays[in.operand1].count || arrays[in.result].real!=arrays[in.operand1].real) goto unsupported;
            for (unsigned k=0;k<arrays[in.result].count;++k) {
                if(arrays[in.result].real) {
                    candidate.loadXmm0FromRbp(arrays[in.operand1].displacement+8*static_cast<int>(k));
                    candidate.storeXmm0AtRbp(arrays[in.result].displacement+8*static_cast<int>(k));
                } else {
                    candidate.loadEaxFromRbp(arrays[in.operand1].displacement+4*static_cast<int>(k));
                    candidate.storeEaxAtRbp(arrays[in.result].displacement+4*static_cast<int>(k));
                }
            }
        } else if (in.opcode=="index" || in.opcode=="index.check" || in.opcode=="index.store :" || in.opcode=="index.store =") {
            const std::string base=in.opcode.find("index.store")==0 ? in.result : in.operand1;
            const std::string index=in.opcode.find("index.store")==0 ? in.operand1 : in.operand2;
            if (!arrays.count(base) || vectorRealKind(index,kinds) || !arrayLoad(candidate,index,slots,initialized)) goto unsupported;
            candidate.checkArrayBounds(arrays[base].count);
            if (in.opcode=="index") {
                if(arrays[base].real) {
                    candidate.loadRealArrayElement(arrays[base].displacement); candidate.storeXmm0AtRbp(slots[in.result]);
                } else {candidate.loadArrayElement(arrays[base].displacement); candidate.storeEaxAtRbp(slots[in.result]);}
                initialized.insert(in.result);
            } else if (in.opcode!="index.check") {
                candidate.pushRax();
                if (!(arrays[base].real ? vectorLoadReal(candidate,in.operand2,slots,initialized,kinds) :
                    (!vectorRealKind(in.operand2,kinds) && arrayLoad(candidate,in.operand2,slots,initialized)))) goto unsupported;
                candidate.popRcx();
                if(arrays[base].real) candidate.storeRealArrayElement(arrays[base].displacement);
                else candidate.storeArrayElement(arrays[base].displacement);
            }
        } else if (in.opcode=="array.len" || in.opcode=="attribute row" || in.opcode=="attribute col" || in.opcode=="attribute dim") {
            if (!arrays.count(in.operand1)) goto unsupported;
            if (in.opcode=="attribute dim") {
                if (!arrayLoad(candidate,in.operand2,slots,initialized)) goto unsupported;
                candidate.arrayDimension(arrays[in.operand1].count);
            } else candidate.movEaxImmediate(in.opcode=="attribute col" ? static_cast<unsigned>(-1) : arrays[in.operand1].count);
            candidate.storeEaxAtRbp(slots[in.result]); initialized.insert(in.result);
        } else if (in.opcode=="unary -") {
            if(kinds[in.result]) {
                if(!vectorLoadReal(candidate,in.operand1,slots,initialized,kinds)) goto unsupported;
                candidate.negateReal(); candidate.storeXmm0AtRbp(slots[in.result]);
            } else {
                if(in.operand1=="2147483648") candidate.movEaxImmediate(0x80000000U);
                else {
                    candidate.movEaxImmediate(0); candidate.pushRax();
                    if(!arrayLoad(candidate,in.operand1,slots,initialized)) goto unsupported;
                    candidate.popRcx(); candidate.subEcxEax(); candidate.checkIntegerOverflow();
                }
                candidate.storeEaxAtRbp(slots[in.result]);
            }
            initialized.insert(in.result);
        } else if (in.opcode=="binary +" || in.opcode=="binary -" || in.opcode=="binary *") {
            if(kinds[in.result]) {
                if(!vectorLoadReal(candidate,in.operand1,slots,initialized,kinds)) goto unsupported;
                candidate.movsdXmm1Xmm0();
                if(!vectorLoadReal(candidate,in.operand2,slots,initialized,kinds)) goto unsupported;
                if(in.opcode=="binary +") candidate.addsdXmm0Xmm1();
                else if(in.opcode=="binary -") {candidate.subsdXmm1Xmm0();candidate.movsdXmm0Xmm1();}
                else candidate.multiplyReal();
                candidate.storeXmm0AtRbp(slots[in.result]); initialized.insert(in.result);
            } else {
                if (!arrayLoad(candidate,in.operand1,slots,initialized)) goto unsupported;
                candidate.pushRax(); if (!arrayLoad(candidate,in.operand2,slots,initialized)) goto unsupported;
                candidate.popRcx();
                if(in.opcode=="binary +")candidate.addEaxEcx();
                else if(in.opcode=="binary -")candidate.subEcxEax();
                else candidate.imulEaxEcx();
                candidate.checkIntegerOverflow(); candidate.storeEaxAtRbp(slots[in.result]); initialized.insert(in.result);
            }
        } else if (in.opcode=="constant" || in.opcode=="initialize" || in.opcode=="assign :" || in.opcode=="assign =") {
            if(kinds[in.result]) {
                if(!vectorLoadReal(candidate,in.operand1,slots,initialized,kinds)) goto unsupported;
                candidate.storeXmm0AtRbp(slots[in.result]);
            } else {
                if(vectorRealKind(in.operand1,kinds) || !arrayLoad(candidate,in.operand1,slots,initialized)) goto unsupported;
                candidate.storeEaxAtRbp(slots[in.result]);
            }
            initialized.insert(in.result);
        } else if (in.opcode=="return") {
            if (!in.operand1.empty()) {
                if(!f.signature.outputs.empty() && f.signature.outputs[0].name=="real") {
                    if(!vectorLoadReal(candidate,in.operand1,slots,initialized,kinds)) goto unsupported;
                } else if(vectorRealKind(in.operand1,kinds) || !arrayLoad(candidate,in.operand1,slots,initialized)) goto unsupported;
            }
            returned=true;
        } else goto unsupported;
        if (debugVariables) for (std::size_t v=0; v<debugVariables->size(); ++v) {
            const std::string& name=(*debugVariables)[v].name;
            if (slots.count(name) && initialized.count(name) && !arrayLiveStarts.count(name))
                arrayLiveStarts[name]=static_cast<unsigned>(candidate.bytes().size());
        }
        continue;
unsupported:
        std::ostringstream detail; detail<<"line "<<in.sourceLine<<": unsupported or uninitialized native numeric-vector operation "<<in.opcode;
        error=detail.str(); return false;
    }
    if (!returned && !f.signature.outputs.empty()) { error="missing native vector return"; return false; }
    if (debugVariables) for (std::size_t v=0; v<debugVariables->size(); ++v) {
        coff::DebugVariable& variable=(*debugVariables)[v];
        if (arrayLiveStarts.count(variable.name) && (arrays.count(variable.name) || slots.count(variable.name))) {
            variable.arrayCount=arrays.count(variable.name) ? arrays[variable.name].count : 0;
            variable.location=coff::DebugVariable::FrameRelative;
            variable.frameOffset=arrays.count(variable.name) ? arrays[variable.name].displacement : slots[variable.name];
            variable.liveStart=arrayLiveStarts[variable.name];
            variable.liveEnd=static_cast<unsigned>(candidate.bytes().size());
            variable.hasLiveRange=true;
        }
    }
    candidate.releaseStack(frame); candidate.popRbp(); candidate.ret();
    relocations.clear(); e=candidate; error.clear(); return true;
}
