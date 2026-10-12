#include "IR.h"
#include <cassert>
#include <vector>
int main() {
    std::vector<shalimar::IRType> outputs;
    shalimar::IRReturnLayout empty = shalimar::IRReturnLayout::fromOutputs(outputs);
    assert(empty.valid && empty.fields.empty() && empty.size == 0);
    outputs.push_back(shalimar::IRType("int", 0, false));
    outputs.push_back(shalimar::IRType("real", 0, false));
    outputs.push_back(shalimar::IRType("char", 0, false));
    shalimar::IRReturnLayout layout = shalimar::IRReturnLayout::fromOutputs(outputs);
    assert(layout.valid && layout.fields.size() == 3);
    assert(layout.fields[0].offset == 0 && layout.fields[0].size == 4);
    assert(layout.fields[1].offset == 8 && layout.fields[1].size == 8);
    assert(layout.fields[2].offset == 16 && layout.fields[2].size == 1);
    assert(layout.size == 24 && layout.alignment == 8);
    outputs.push_back(shalimar::IRType("char", 1, false));
    layout = shalimar::IRReturnLayout::fromOutputs(outputs);
    assert(!layout.valid && layout.fields.empty());
    return 0;
}
