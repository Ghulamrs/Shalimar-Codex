#include "backend/coff/CoffWriter.h"
#include <fstream>
#include <algorithm>
#include <map>
#include <set>
namespace shalimar { namespace coff {
namespace {
void u16(std::vector<unsigned char>& b, unsigned v) { b.push_back(static_cast<unsigned char>(v)); b.push_back(static_cast<unsigned char>(v >> 8)); }
void u32(std::vector<unsigned char>& b, unsigned v) { for (unsigned i=0;i<4;++i) b.push_back(static_cast<unsigned char>(v >> (8*i))); }
void name8(std::vector<unsigned char>& b, const std::string& s) { for (unsigned i=0;i<8;++i) b.push_back(i<s.size()?static_cast<unsigned char>(s[i]):0); }
void set32(std::vector<unsigned char>& b, unsigned p, unsigned v) { for(unsigned i=0;i<4;++i) b[p+i]=static_cast<unsigned char>(v>>(8*i)); }
void pad4(std::vector<unsigned char>& b) { while(b.size()%4) b.push_back(0); }
void subsection(std::vector<unsigned char>& b, unsigned type, const std::vector<unsigned char>& payload) {
    u32(b,type); u32(b,static_cast<unsigned>(payload.size())); b.insert(b.end(),payload.begin(),payload.end()); pad4(b);
}
struct DebugReloc { unsigned offset; unsigned symbol; unsigned short type; DebugReloc(unsigned o,unsigned s,unsigned short t):offset(o),symbol(s),type(t){} };
bool relocLess(const DebugReloc& a, const DebugReloc& b) { return a.offset<b.offset; }
}
bool CoffWriter::write(const std::vector<FunctionBytes>& fs, const std::string& path, std::string& error, bool codeView) const {
    if (fs.empty()) { error="no functions to emit"; return false; }
    std::set<std::string> names; std::vector<unsigned char> code; std::vector<unsigned> offsets;
    for(std::size_t i=0;i<fs.size();++i) {
        if(fs[i].name.empty() || !names.insert(fs[i].name).second || fs[i].code.empty()) {error="invalid or duplicate function";return false;}
        if(code.size()>0xffffffffUL-fs[i].code.size()) {error="COFF size limit";return false;}
        offsets.push_back(static_cast<unsigned>(code.size())); code.insert(code.end(),fs[i].code.begin(),fs[i].code.end());
    }
    std::set<std::string> externals; unsigned relocationCount=0;
    for(std::size_t i=0;i<fs.size();++i) for(std::size_t j=0;j<fs[i].relocations.size();++j) {
        const Relocation& r=fs[i].relocations[j];
        if(r.symbol.empty() || r.offset>fs[i].code.size() || fs[i].code.size()-r.offset<4 || r.type!=0x0004) {error="invalid REL32 relocation";return false;}
        if(relocationCount==65535U) {error="too many COFF relocations";return false;}
        ++relocationCount; if(!names.count(r.symbol)) externals.insert(r.symbol);
    }
    std::vector<std::string> symbols;
    for(std::size_t i=0;i<fs.size();++i) symbols.push_back(fs[i].name);
    for(std::set<std::string>::const_iterator it=externals.begin();it!=externals.end();++it) symbols.push_back(*it);
    std::map<std::string,unsigned> symbolIndex;
    for(std::size_t i=0;i<symbols.size();++i) symbolIndex[symbols[i]]=static_cast<unsigned>(i);
    // Emit only verified frame-relative locations. Register mapping is not yet
    // qualified against the x64 selector and is deliberately rejected.
    if (codeView) for (std::size_t fi=0; fi<fs.size(); ++fi)
        for (std::size_t vi=0; vi<fs[fi].debugVariables.size(); ++vi) {
            const DebugVariable& var=fs[fi].debugVariables[vi];
            if (var.name.empty() || var.typeName.empty()) {
                error="invalid CodeView variable metadata"; return false;
            }
            if (var.typeName!="int" && var.typeName!="real" && var.typeName!="char") {
                error="unsupported CodeView variable type"; return false;
            }
            if (var.location==DebugVariable::Register) {
                error="CodeView register mapping not qualified"; return false;
            }
        }
    std::vector<unsigned char> debug; std::vector<DebugReloc> debugRelocs;
    if(codeView) {
        u32(debug,4); // C13 signature
        std::vector<unsigned char> strings(1,0); std::map<std::string,unsigned> fileOffsets;
        for(std::size_t i=0;i<fs.size();++i) for(std::size_t j=0;j<fs[i].debugLines.size();++j) {
            const DebugLine& line=fs[i].debugLines[j];
            if(line.file.empty() || line.line==0 || line.offset>=fs[i].code.size()) {error="invalid CodeView source mark";return false;}
            if(!fileOffsets.count(line.file)) {
                fileOffsets[line.file]=static_cast<unsigned>(strings.size());
                for(std::size_t k=0;k<line.file.size();++k) strings.push_back(static_cast<unsigned char>(line.file[k]));
                strings.push_back(0);
            }
        }
        subsection(debug,0xF3,strings); // DEBUG_S_STRINGTABLE
        std::vector<unsigned char> checksums; std::map<std::string,unsigned> checksumOffsets;
        for(std::map<std::string,unsigned>::const_iterator it=fileOffsets.begin();it!=fileOffsets.end();++it) {
            checksumOffsets[it->first]=static_cast<unsigned>(checksums.size());
            u32(checksums,it->second); checksums.push_back(0); checksums.push_back(0); // no checksum
            while(checksums.size()%4) checksums.push_back(0);
        }
        subsection(debug,0xF4,checksums); // DEBUG_S_FILECHKSMS
        // C13 compilation-unit identity records. These do not claim that
        // local-variable locations or user-defined type records exist yet.
        // S_OBJNAME: signature followed by NUL-terminated object identity.
        // S_COMPILE3: machine/language and tool version metadata.
        {
            std::vector<unsigned char> unit;
            std::vector<unsigned char> record;
            u16(record,0); u16(record,0x1101); // S_OBJNAME
            u32(record,0); // signature
            const std::string objName("shalimar-generated.obj");
            record.insert(record.end(),objName.begin(),objName.end());
            record.push_back(0);
            if(record.size()-2>65535U) {error="CodeView object-name record too long";return false;}
            record[0]=static_cast<unsigned char>((record.size()-2)&255);
            record[1]=static_cast<unsigned char>((record.size()-2)>>8);
            unit.insert(unit.end(),record.begin(),record.end());
            // S_COMPILE3: flags, machine AMD64, frontend/backend versions,
            // followed by a NUL-terminated compiler-version string.
            record.clear();
            u16(record,0); u16(record,0x113c);
            u32(record,0); // C/C++ language flags (no false language claim)
            u16(record,0x00d0); // CV_CFL_X64
            for(unsigned v=0;v<8;++v) u16(record,0); // FE and BE versions
            const std::string compilerName("Shalimar C++98 compiler Rev54-D3");
            record.insert(record.end(),compilerName.begin(),compilerName.end());
            record.push_back(0);
            if(record.size()-2>65535U) {error="CodeView compile record too long";return false;}
            record[0]=static_cast<unsigned char>((record.size()-2)&255);
            record[1]=static_cast<unsigned char>((record.size()-2)>>8);
            unit.insert(unit.end(),record.begin(),record.end());
            subsection(debug,0xF1,unit);
        }
        // C13 DEBUG_S_SYMBOLS: one procedure scope per emitted function.
        // S_GPROC32_ID records use a section-relative relocation for the
        // function start and an IMAGE_REL_AMD64_SECTION relocation for segment.
        for(std::size_t i=0;i<fs.size();++i) {
            std::vector<unsigned char> syms;
            const unsigned procStart=static_cast<unsigned>(syms.size());
            u16(syms,0); u16(syms,0x1147); // S_GPROC32_ID
            u32(syms,0); // parent scope
            const unsigned endField=static_cast<unsigned>(syms.size());
            u32(syms,0); // offset of matching S_END within this subsection
            u32(syms,0); // next scope
            u32(syms,static_cast<unsigned>(fs[i].code.size())); // code length
            u32(syms,0); // debug start
            u32(syms,static_cast<unsigned>(fs[i].code.size())); // debug end
            u32(syms,0); // type index: no argument/type records yet
            const unsigned addressField=static_cast<unsigned>(syms.size());
            u32(syms,0); // relocated procedure offset
            u16(syms,0); // relocated section number
            syms.push_back(0); // procedure flags
            for(std::size_t c=0;c<fs[i].name.size();++c)
                syms.push_back(static_cast<unsigned char>(fs[i].name[c]));
            syms.push_back(0);
            while((syms.size()-procStart)%4) syms.push_back(0);
            const unsigned procSize=static_cast<unsigned>(syms.size()-procStart);
            if(procSize<4 || procSize-2>65535U) {error="CodeView procedure record too long";return false;}
            syms[procStart]=static_cast<unsigned char>((procSize-2)&255);
            syms[procStart+1]=static_cast<unsigned char>((procSize-2)>>8);
            // S_REGREL32 uses an actual selector-provided frame offset.
            // Primitive CodeView type indices: T_INT4=0x74, T_REAL64=0x41,
            // T_CHAR=0x70. CV_AMD64_RBP=334. No fabricated offsets.
            for (std::size_t vi=0; vi<fs[i].debugVariables.size(); ++vi) {
                const DebugVariable& var=fs[i].debugVariables[vi];
                if (var.location!=DebugVariable::FrameRelative) continue;
                const unsigned typeIndex=var.typeName=="int" ? 0x74U :
                    (var.typeName=="real" ? 0x41U : 0x70U);
                if (var.hasLiveRange) {
                    // S_LOCAL followed by S_DEFRANGE_FRAMEPOINTER_REL.
                    // A variable is available only within its verified interval.
                    if (var.liveStart>=var.liveEnd || var.liveEnd>fs[i].code.size() ||
                        var.liveEnd-var.liveStart>65535U) {
                        error="invalid CodeView local live range"; return false;
                    }
                    const unsigned localStart=static_cast<unsigned>(syms.size());
                    u16(syms,0); u16(syms,0x113e); // S_LOCAL
                    u32(syms,typeIndex); u16(syms,0); // flags
                    syms.insert(syms.end(),var.name.begin(),var.name.end()); syms.push_back(0);
                    const unsigned localSize=static_cast<unsigned>(syms.size()-localStart-2);
                    if(localSize>65535U) {error="CodeView S_LOCAL too long";return false;}
                    syms[localStart]=static_cast<unsigned char>(localSize);
                    syms[localStart+1]=static_cast<unsigned char>(localSize>>8);
                    const unsigned rangeStart=static_cast<unsigned>(syms.size());
                    u16(syms,0); u16(syms,0x1142); // S_DEFRANGE_FRAMEPOINTER_REL
                    u32(syms,static_cast<unsigned>(var.frameOffset));
                    const unsigned addressStart=static_cast<unsigned>(syms.size());
                    u32(syms,var.liveStart); // function-relative start, relocated via SECREL
                    u16(syms,0); // section, relocated via SECTION
                    u16(syms,var.liveEnd-var.liveStart);
                    const unsigned rangeSize=static_cast<unsigned>(syms.size()-rangeStart-2);
                    syms[rangeStart]=static_cast<unsigned char>(rangeSize);
                    syms[rangeStart+1]=static_cast<unsigned char>(rangeSize>>8);
                    // Subsection starts with an 8-byte C13 header.
                    const unsigned relocBase=static_cast<unsigned>(debug.size())+8;
                    debugRelocs.push_back(DebugReloc(relocBase+addressStart,static_cast<unsigned>(i),0x000B));
                    debugRelocs.push_back(DebugReloc(relocBase+addressStart+4,static_cast<unsigned>(i),0x000A));
                    continue;
                }
                const unsigned recordStart=static_cast<unsigned>(syms.size());
                u16(syms,0); u16(syms,0x1111); // S_REGREL32
                u32(syms,static_cast<unsigned>(var.frameOffset));
                u32(syms,typeIndex);
                u16(syms,334); // CV_AMD64_RBP
                syms.insert(syms.end(),var.name.begin(),var.name.end());
                syms.push_back(0);
                if (syms.size()-recordStart-2>65535U) {
                    error="CodeView local record too long"; return false;
                }
                const unsigned recordSize=static_cast<unsigned>(syms.size()-recordStart-2);
                syms[recordStart]=static_cast<unsigned char>(recordSize&255);
                syms[recordStart+1]=static_cast<unsigned char>(recordSize>>8);
            }
            const unsigned endStart=static_cast<unsigned>(syms.size());
            set32(syms,endField,endStart);
            u16(syms,2); u16(syms,0x0006); // S_END
            const unsigned base=static_cast<unsigned>(debug.size());
            subsection(debug,0xF1,syms); // DEBUG_S_SYMBOLS
            debugRelocs.push_back(DebugReloc(base+8+addressField,static_cast<unsigned>(i),0x000B));
            debugRelocs.push_back(DebugReloc(base+8+addressField+4,static_cast<unsigned>(i),0x000A));
        }
        for(std::size_t i=0;i<fs.size();++i) {
            // DEBUG_S_LINES: contribution relocation + file blocks
            std::vector<unsigned char> lines; u32(lines,0); u16(lines,0); u16(lines,0); u32(lines,static_cast<unsigned>(fs[i].code.size()));
            std::map<std::string,std::vector<DebugLine> > groups;
            for(std::size_t j=0;j<fs[i].debugLines.size();++j) groups[fs[i].debugLines[j].file].push_back(fs[i].debugLines[j]);
            for(std::map<std::string,std::vector<DebugLine> >::const_iterator it=groups.begin();it!=groups.end();++it) {
                u32(lines,checksumOffsets[it->first]); u32(lines,static_cast<unsigned>(it->second.size()));
                u32(lines,static_cast<unsigned>(it->second.size()*8));
                for(std::size_t j=0;j<it->second.size();++j) {
                    u32(lines,it->second[j].offset); u32(lines,it->second[j].line | 0x80000000U);
                }
            }
            const unsigned base=static_cast<unsigned>(debug.size());
            subsection(debug,0xF2,lines);
            debugRelocs.push_back(DebugReloc(base+8,static_cast<unsigned>(i),0x000B)); // AMD64_SECREL
            debugRelocs.push_back(DebugReloc(base+12,static_cast<unsigned>(i),0x000A)); // AMD64_SECTION
        }
        std::sort(debugRelocs.begin(),debugRelocs.end(),relocLess);
        if(debugRelocs.size()>65535U) {error="too many CodeView relocations";return false;}
    }
    std::vector<unsigned char> strtab; u32(strtab,4); std::map<std::string,unsigned> longNames;
    for(std::size_t i=0;i<symbols.size();++i) if(symbols[i].size()>8) {
        longNames[symbols[i]]=static_cast<unsigned>(strtab.size());
        for(std::size_t j=0;j<symbols[i].size();++j) strtab.push_back(static_cast<unsigned char>(symbols[i][j])); strtab.push_back(0);
    }
    set32(strtab,0,static_cast<unsigned>(strtab.size()));
    const unsigned headerSize=20+(codeView?80:40);
    const unsigned textRaw=headerSize;
    const unsigned textRel=textRaw+static_cast<unsigned>(code.size());
    const unsigned debugRaw=textRel+relocationCount*10U;
    const unsigned debugRel=debugRaw+static_cast<unsigned>(debug.size());
    const unsigned symbolOffset=debugRel+static_cast<unsigned>(debugRelocs.size()*10U);
    std::vector<unsigned char> b;
    u16(b,0x8664); u16(b,codeView?2:1); u32(b,0); u32(b,symbolOffset);
    u32(b,static_cast<unsigned>(symbols.size())); u16(b,0); u16(b,0);
    name8(b,".text"); u32(b,0); u32(b,0); u32(b,static_cast<unsigned>(code.size()));
    u32(b,textRaw); u32(b,relocationCount?textRel:0); u32(b,0);
    u16(b,relocationCount); u16(b,0); u32(b,0x60500020);
    if(codeView) {
        name8(b,".debug$S"); u32(b,0); u32(b,0); u32(b,static_cast<unsigned>(debug.size()));
        u32(b,debugRaw); u32(b,debugRelocs.empty()?0:debugRel); u32(b,0);
        u16(b,static_cast<unsigned>(debugRelocs.size())); u16(b,0); u32(b,0x42100040);
    }
    b.insert(b.end(),code.begin(),code.end());
    for(std::size_t i=0;i<fs.size();++i) for(std::size_t j=0;j<fs[i].relocations.size();++j) {
        const Relocation& r=fs[i].relocations[j]; u32(b,offsets[i]+r.offset); u32(b,symbolIndex[r.symbol]); u16(b,r.type);
    }
    b.insert(b.end(),debug.begin(),debug.end());
    for(std::size_t i=0;i<debugRelocs.size();++i) {
        u32(b,debugRelocs[i].offset); u32(b,debugRelocs[i].symbol); u16(b,debugRelocs[i].type);
    }
    for(std::size_t i=0;i<symbols.size();++i) {
        if(symbols[i].size()<=8) name8(b,symbols[i]); else {u32(b,0);u32(b,longNames[symbols[i]]);}
        const bool defined=names.count(symbols[i])!=0; unsigned value=0;
        if(defined) for(std::size_t j=0;j<fs.size();++j) if(fs[j].name==symbols[i]) {value=offsets[j];break;}
        u32(b,value); u16(b,defined?1:0); u16(b,defined?0x20:0); b.push_back(2); b.push_back(0);
    }
    b.insert(b.end(),strtab.begin(),strtab.end());
    std::ofstream out(path.c_str(),std::ios::binary|std::ios::trunc);
    if(!out) {error="cannot open output COFF file";return false;}
    out.write(reinterpret_cast<const char*>(&b[0]),static_cast<std::streamsize>(b.size()));
    if(!out.good()) {error="cannot write COFF file";return false;}
    error.clear();return true;
}
}}
