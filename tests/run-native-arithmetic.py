"""Execute native overflow boundaries in isolated children (Windows and DSP)."""
import argparse, ctypes, json, os, pathlib, subprocess, sys

def invoke(command):
    return subprocess.run(list(map(str, command)), capture_output=True, text=True, timeout=30)

if len(sys.argv) > 1 and sys.argv[1] == '--child':
    class Pair(ctypes.Structure):
        _fields_ = [('value', ctypes.c_int32), ('zero', ctypes.c_int32)]
    library = ctypes.CDLL(sys.argv[2])
    library.compute.argtypes = [ctypes.POINTER(Pair), ctypes.c_int32]
    library.compute.restype = None
    result = Pair()
    library.compute(ctypes.byref(result), int(sys.argv[3]))
    print(result.value)
    raise SystemExit(0)

parser = argparse.ArgumentParser()
parser.add_argument('--compiler-build', required=True)
parser.add_argument('--output', required=True)
parser.add_argument('--configuration', choices=['Debug', 'Release'], required=True)
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parents[1]
build = pathlib.Path(args.compiler_build).resolve()
compiler = build / args.configuration / 'shalimarc.exe'
runtime = build / args.configuration / 'shalimar_runtime.lib'
out = pathlib.Path(args.output).resolve(); out.mkdir(parents=True, exist_ok=True)
tools = root / 'toolsets/shalimar-c6747'
msvc = pathlib.Path(os.environ.get('ProgramFiles', 'C:/Program Files')) / 'Microsoft Visual Studio/2022/Community/VC/Tools/MSVC'
link = sorted(msvc.glob('*/bin/Hostx64/x64/link.exe'))[-1]
sdk = pathlib.Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Windows Kits/10/Lib'
kernel = sorted(sdk.glob('*/um/x64/kernel32.lib'))[-1]
cases = [
    ('x + 1', 2147483647, None), ('x - 1', -2147483648, None),
    ('x * 2', 2147483647, None), ('x * 2', -2147483648, None),
    ('(x + 1) - 1', 2147483647, None),
    ('x + 1', 2147483646, 2147483647), ('x - 1', -2147483647, -2147483648),
    ('x * 2', -1073741824, -2147483648), ('x * 2', 1073741823, 2147483646),
    ('x * 0', 2147483647, 0), ('x + 1', -2, -1), ('x - 1', 0, -1),
]
results = []
for mode in ('debug', 'release'):
    for index, (expression, value, expected) in enumerate(cases):
        stem = out / f'{args.configuration}-{mode}-{index}'
        source = stem.with_suffix('.shl')
        source.write_text('fun <int,int> = compute(x: int) {\nreturn (' + expression + ', 0)\n}\nfun main() {}\n')
        obj, dll = stem.with_suffix('.obj'), stem.with_suffix('.dll')
        p = invoke([compiler, source, '--object', obj, '--'+mode]); assert p.returncode == 0, p.stderr
        p = invoke([link, '/nologo', '/dll', '/noentry', '/nodefaultlib', '/export:compute', '/out:'+str(dll), obj, runtime, kernel])
        assert p.returncode == 0, p.stdout+p.stderr
        p = invoke([sys.executable, __file__, '--child', dll, value])
        success = (p.returncode == 1 and p.stdout == '' and p.stderr == 'Error: line 2: integer overflow\n') if expected is None else (p.returncode == 0 and p.stdout.strip() == str(expected) and p.stderr == '')
        results.append(dict(target='windows', mode=mode, expression=expression, input=value, expected=expected, success=success, exit=p.returncode, stdout=p.stdout, stderr=p.stderr))
        assert success, results[-1]
        source.write_text('fun <int> = compute(x: int) {\nreturn (' + expression + ')\n}\nfun <int> = answer() { return compute(' + str(value) + ') }\nfun main() {}\n')
        exe = stem.with_suffix('.out')
        p = invoke([compiler, source, '--target=tms6747', '--tools-root', tools, '--rts-lib', tools/'lib/rts6x.lib', '--dsp-entry', 'answer', '--'+mode, '-o', exe])
        assert p.returncode == 0, p.stdout+p.stderr
        p = invoke([tools/'bin/sim6747.exe', '--run', exe, '--main-status', '-c'])
        combined = p.stdout+p.stderr
        status = 1 if expected is None else expected
        success = p.returncode == (status & 255) and f'exit=C$$EXIT status={status}' in combined
        if expected is None: success = success and 'Error: line 2: integer overflow' in combined
        else: success = success and 'Error:' not in combined
        results.append(dict(target='tms6747', mode=mode, expression=expression, input=value, expected=expected, success=success, exit=p.returncode, stdout=p.stdout, stderr=p.stderr))
        assert success, results[-1]
(out/'results.json').write_text(json.dumps(results, indent=2))
print(f'{len(results)}/{len(results)} native arithmetic boundary cases passed ({args.configuration} host)')
