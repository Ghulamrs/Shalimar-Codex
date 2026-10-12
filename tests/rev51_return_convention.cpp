#include "backend/common/ReturnConvention.h"
#include <cassert>
int main() {
    const shalimar::ReturnConvention win = shalimar::returnConvention("windows-x86_64");
    const shalimar::ReturnConvention linux = shalimar::returnConvention("linux-x86_64");
    const shalimar::ReturnConvention mac = shalimar::returnConvention("macos-arm64");
    const shalimar::ReturnConvention dsp = shalimar::returnConvention("tms6747");
    assert(win.integerRegister == "EAX" && win.realRegister == "XMM0");
    assert(win.charRegister == "AL" && win.indirectResultRegister == "RCX");
    assert(linux.indirectResultRegister == "RDI");
    assert(mac.integerRegister == "W0" && mac.realRegister == "D0");
    assert(mac.indirectResultRegister == "X8");
    assert(dsp.integerRegister == "A4" && dsp.realRegister == "A5:A4");
    assert(dsp.indirectResultRegister == "ABI_UNVERIFIED");
    assert(!shalimar::usesStructuredReturn(1));
    assert(shalimar::usesStructuredReturn(2));
    assert(!mac.verifiedNativeImplementation && !dsp.verifiedNativeImplementation);
}
