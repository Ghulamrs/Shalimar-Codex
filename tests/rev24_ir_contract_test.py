#!/usr/bin/env python3
"""Static rev24 contract checkpoint; not a substitute for an IR verifier."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
contract = (root / 'docs/REV24_IR_CONTRACT.md').read_text()
for required in ('Multi-output values', 'dominance', 'Array indexing', 'Structural verifier', 'native backend'):
    assert required.lower() in contract.lower(), required
header = (root / 'include/IR.h').read_text()
assert 'struct IRInstruction' in header and 'std::vector<IRBasicBlock>' in header
print('PASS: rev24 contract document and legacy IR baseline present')
