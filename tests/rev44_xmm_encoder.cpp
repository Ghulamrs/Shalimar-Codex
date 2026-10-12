#include "backend/x86_64/InstructionEncoder.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <iostream>
int main() {
    shalimar::x86_64::InstructionEncoder e;
    const double v=3.5;
    unsigned words[2]={0,0};
    assert(sizeof(v)==sizeof(words));
    std::memcpy(words,&v,sizeof(words));
    e.movRaxImmediate64(words[0],words[1]);
    e.movqXmm0Rax();
    e.movsdXmm1Xmm0();
    e.addsdXmm0Xmm1();
    e.ret();
    const unsigned char expected[]={0x48,0xb8,0,0,0,0,0,0,0x0c,0x40,
      0x66,0x48,0x0f,0x6e,0xc0,0xf2,0x0f,0x10,0xc8,0xf2,0x0f,0x58,0xc1,0xc3};
    const std::vector<unsigned char>& code=e.bytes();
    assert(code.size()==sizeof(expected));
    for (std::size_t i=0;i<sizeof(expected);++i) assert(code[i]==expected[i]);
    std::cout << "rev44 XMM binary64 encoding passed\n";
}
