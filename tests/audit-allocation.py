#!/usr/bin/env python3
"""Audit compiler code tokens, separately reporting emitted-runtime text."""
import json
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parent.parent
forbidden = re.compile(r'\b(?:new|delete)\b|\b(?:malloc|calloc|realloc|free)\s*\(')
# C++98 source has no raw string literals. Preserve line breaks for locations.
noncode = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
violations, emitted_text = [], []
for path in sorted(list((root / 'src').glob('*.cpp')) + list((root / 'include').glob('*.h'))):
    source = path.read_text()
    code = noncode.sub(lambda m: '\n' * m.group().count('\n') + ' ', source)
    for match in forbidden.finditer(code):
        violations.append({'file': str(path.relative_to(root)), 'line': code[:match.start()].count('\n') + 1, 'token': match.group()})
    for match in re.finditer(r'"(?:\\.|[^"\\])*"', source):
        if forbidden.search(match.group()):
            emitted_text.append({'file': str(path.relative_to(root)), 'line': source[:match.start()].count('\n') + 1})
result = {'compiler_code_violations': violations, 'generated_C_runtime_strings_with_allocation_terms': emitted_text,
          'scope': 'src/*.cpp and include/*.h; comments and string literals excluded from executable compiler-code tokens'}
print(json.dumps(result, indent=2))
sys.exit(bool(violations))
