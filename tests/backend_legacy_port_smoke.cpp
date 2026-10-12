#include "../src/backend/legacy_port/X86_64Windows.h"
#include "../src/backend/legacy_port/X86_64Linux.h"
#include "../src/backend/legacy_port/Arm64Darwin.h"
#include "../src/backend/legacy_port/Tms6747.h"
#include <iostream>
#include <string>
static bool check(shalimar::Emitter &e, const std::string &needle) {
    e.beginModule("smoke.shm"); e.beginFunction("smoke");
    e.loadIntConstant(7); e.endFunction(0); e.endModule();
    if (e.text().find(needle) == std::string::npos) {
        std::cerr << "missing marker: " << needle << "\n"; return false;
    }
    return true;
}
int main() {
    shalimar::X86_64WindowsEmitter win;
    shalimar::X86_64LinuxEmitter linux;
    shalimar::Arm64DarwinEmitter mac;
    shalimar::Tms6747Emitter dsp;
    return check(win,"smoke") && check(linux,"smoke") && check(mac,"_smoke") && check(dsp,"smoke") ? 0 : 1;
}
