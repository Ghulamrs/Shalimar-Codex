#include "backend/coff/CoffWriter.h"
#include <iostream>
#include <vector>
int main(int argc,char** argv) {
 if(argc!=2)return 2;
 shalimar::coff::FunctionBytes f; f.name="caller";
 const unsigned char bytes[]={0xe8,0,0,0,0,0xe8,0,0,0,0,0xc3};
 f.code.assign(bytes,bytes+11);
 shalimar::coff::Relocation a; a.offset=1; a.symbol="callee"; f.relocations.push_back(a);
 shalimar::coff::Relocation b; b.offset=6; b.symbol="external_long_name"; f.relocations.push_back(b);
 shalimar::coff::FunctionBytes g; g.name="callee"; g.code.push_back(0xc3);
 std::vector<shalimar::coff::FunctionBytes> functions; functions.push_back(f);functions.push_back(g);
 std::string error;
 if(!shalimar::coff::CoffWriter().write(functions,argv[1],error)) {std::cerr<<error;return 1;}
 f.relocations[0].offset=9; functions[0]=f;
 if(shalimar::coff::CoffWriter().write(functions,std::string(argv[1])+".bad",error))return 3;
 return 0;
}
