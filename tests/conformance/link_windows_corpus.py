import json
import pathlib
import re
import subprocess
import os

root = pathlib.Path(__file__).resolve().parents[2]
out = root / 'build/vs2022/qualification'
tools = pathlib.Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64')
report = json.loads((out / 'corpus-results.json').read_text())
results = []
for row in report['results']:
    if row['stage'] != 'object' or not row['success']:
        continue
    obj = root / row['artifact']
    symbols = subprocess.run([str(tools / 'dumpbin.exe'), '/symbols', str(obj)], capture_output=True, text=True, check=True).stdout
    exports = re.findall(r'SECT\d+.*?External\s+\|\s+(\w+)', symbols)
    dll = obj.with_suffix('.dll')
    args = [str(tools / 'link.exe'), '/nologo', '/dll', '/noentry', '/nodefaultlib', '/machine:x64',
            '/out:' + str(dll), str(obj)] + ['/export:' + name for name in exports]
    runtime = root / 'build/vs2022' / row['configuration'] / 'shalimar_runtime.lib'
    kernel = sorted(pathlib.Path(os.environ.get('ProgramFiles(x86)', r'C:/Program Files (x86)')).joinpath('Windows Kits/10/Lib').glob('*/um/x64/kernel32.lib'))[-1]
    args.extend([str(runtime), str(kernel)])
    if re.search(r'UNDEF.*?External\s+\|\s+sqrt\b', symbols):
        args.append(str(root / 'build/vs2022/runtime/runtime.obj'))
    proc = subprocess.run(args, capture_output=True, text=True, timeout=30)
    obj.with_suffix('.link.log').write_text(proc.stdout + proc.stderr)
    results.append(dict(configuration=row['configuration'], mode=row['mode'], source=row['source'],
        exports=exports, success=proc.returncode == 0 and dll.exists(), exit_code=proc.returncode,
        diagnostic=proc.stdout + proc.stderr, dll=str(dll.relative_to(root))))
(out / 'link-results.json').write_text(json.dumps(results, indent=2) + '\n')
print(f"Native DLL links: {sum(r['success'] for r in results)}/{len(results)}")
for row in results:
    if not row['success']: print(row['source'], row['diagnostic'])
