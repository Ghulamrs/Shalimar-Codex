#!/usr/bin/env python3
"""Honest P0-P3 S-native conformance evidence; no host-C fallback."""
import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import tempfile

p = argparse.ArgumentParser()
p.add_argument('compiler', type=pathlib.Path)
p.add_argument('--output', type=pathlib.Path)
a = p.parse_args()
root = pathlib.Path(__file__).resolve().parents[2]
compiler = str(a.compiler.resolve())
results = []

def run(cmd):
    return subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def record(category, name, result, detail=''):
    results.append({'category':category, 'name':name, 'result':result, 'detail':detail[:350]})

with tempfile.TemporaryDirectory(prefix='shalimar-rev49-') as td:
    temp = pathlib.Path(td)
    for source in sorted((root/'tests/spec-examples').glob('*.shm')):
        check = run([compiler, str(source), '--check'])
        record('P0 frontend examples', source.name, 'PASS' if check.returncode == 0 else 'FAIL', check.stderr)
        for mode in ('debug','release'):
            obj = temp/(source.stem+'-'+mode+'.obj')
            native = run([compiler, str(source), '--object', str(obj), '--'+mode])
            record('P0 Windows COFF native '+mode, source.name,
                   'PASS' if native.returncode == 0 and obj.is_file() and obj.stat().st_size else 'GAP', native.stderr)
    for case in json.loads((root/'tests/conformance/negative_cases.json').read_text()):
        source = temp/(case['id']+'.shl')
        source.write_text(case['source']+'\n')
        check = run([compiler, str(source), '--check'])
        record('P0 negative semantics', case['id'], 'PASS' if check.returncode != 0 else 'FAIL', check.stderr)
    for name, args in [
        ('CodeView/PDB', ['--debug-symbols']),
        ('Linux ELF', ['--target=x86_64-linux-gnu']),
        ('macOS Mach-O', ['--target=arm64-apple-darwin']),
        ('TI TMS6747', ['--target=tms6747'])
    ]:
        source = root/'tests/rev45_sqrt_literal.shl'
        obj = temp/(name.replace('/','-')+'.obj')
        proc = run([compiler,str(source),'--object',str(obj)]+args)
        record('P0-P3 target and debug gates',name,'GAP' if proc.returncode != 0 else 'NEEDS REVIEW',proc.stderr)

sources = list((root/'src').rglob('*.cpp')) + list((root/'include').rglob('*.h'))
violations = []
for f in sources:
    txt = f.read_text(errors='replace')
    stripped = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"', '', txt, flags=re.S)
    for pattern in (r'\bnew\s+(?:[A-Za-z_]\w*::)*[A-Za-z_]\w*\s*[\[(]|\bnew\s*\(', r'\bdelete\s*(?:\[|[A-Za-z_]\w*)',r'\bmalloc\s*\(',r'\bfree\s*\('):
        if re.search(pattern,stripped):
            violations.append(f.relative_to(root).as_posix()+': '+pattern)
record('Architecture', 'source allocation-token scan', 'PASS' if not violations else 'REVIEW', '; '.join(violations))
ref = root/'docs/SHALIMAR_LANGUAGE.md'
record('Specification','uploaded 3.0 reference SHA256','RECORDED',hashlib.sha256(ref.read_bytes()).hexdigest())
counts = {}
for r in results:
    counts[r['result']] = counts.get(r['result'],0)+1
report = {'revision':'rev49','method':'real compiler invocations; GAP means unsupported, not a test pass',
          'counts':counts,'results':results}
out = a.output or root/'tests/conformance/rev49_results.json'
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(report,indent=2)+'\n')
for r in results:
    print('{:13s} {:34s} {}'.format(r['result'],r['category'],r['name']))
print('Summary:', counts)
if any(r['result']=='FAIL' for r in results):
    raise SystemExit(1)
