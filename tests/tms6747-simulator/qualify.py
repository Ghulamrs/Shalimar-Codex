import argparse
import json
import pathlib
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--quick', action='store_true')
parser.add_argument('--compiler-build', default='build/vs2022', help='Host compiler build directory, relative to project root or absolute')
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parents[2]
out = root / 'build/tms6747-rev3'
out.mkdir(parents=True, exist_ok=True)
dss = r'C:/ti/ccsv5/ccs_base/scripting/bin/dss.bat'
cases = [('return_42', 42, 'int'), ('scalar_calls', 42, 'int'),
         ('scalar_loop', 42, 'int'), ('scalar_multiply', 42, 'int'),
         ('real_arithmetic', -0.75, 'real'), ('mixed_real', 3.5, 'real'),
         ('ten_args', 55, 'int'), ('ten_real_args', 60, 'real'),
         ('recursion', 720, 'int'), ('nested_calls', 42, 'int'),
         ('signed_comparisons', 42, 'int'), ('real_multiply', 3.375, 'real')]
results = []
def run(command, log):
    process = subprocess.run(command, capture_output=True, text=True, timeout=120, cwd=root)
    text = process.stdout + process.stderr
    log.write_text(text, encoding='utf-8')
    return process.returncode, text
for configuration in (('Debug',) if args.quick else ('Debug', 'Release')):
    for mode in ('debug', 'release'):
        for name, expected, kind in cases:
            label = configuration + '-' + mode + '-' + name
            executable = out / (label + '.out')
            source = root / 'tests/tms6747' / (name + '.shl')
            compiler = root / args.compiler_build / configuration / 'shalimarc.exe'
            code, diagnostic = run([str(compiler), source.as_posix(), '--target=tms6747', '--toolchain=ti', '--' + mode,
                '--debug-symbols', '--dsp-entry', 'answer', '-o', str(executable)], out / (label + '.build.log'))
            success = False
            if code == 0:
                simcode, diagnostic = run([dss, str(root / 'tests/tms6747-simulator/run-shalimar.js'),
                    root.as_posix(), executable.as_posix(), str(expected), kind], out / (label + '.sim.log'))
                success = simcode == 0 and 'PASS: Shalimar-generated DSP function' in diagnostic and 'FAIL' not in diagnostic
            results.append(dict(configuration=configuration, mode=mode, case=name, expected=expected,
                kind=kind, build_exit=code, success=success, diagnostic=diagnostic))
            print(label, 'PASS' if success else 'FAIL', flush=True)
        for name, kind in [('scalar_calls', 'int'), ('real_arithmetic', 'real'),
                           ('ten_args', 'ten-int'), ('ten_real_args', 'ten-real')]:
            label = configuration + '-' + mode + '-' + name
            executable = out / (label + '.out')
            code, diagnostic = run([dss, str(root / 'tests/tms6747-simulator/debug-shalimar.js'),
                root.as_posix(), executable.as_posix(), (root / 'tests/tms6747' / (name + '.shl')).as_posix(), kind],
                out / (label + '.debugger.log'))
            success = code == 0 and 'PASS: source breakpoint, parameters, local, call stack, continuation' in diagnostic and 'FAIL' not in diagnostic
            results.append(dict(configuration=configuration, mode=mode, case='source-debugger-' + kind, success=success, diagnostic=diagnostic))
            print(configuration, mode, 'source-debugger-' + kind, 'PASS' if success else 'FAIL', flush=True)
(out / 'qualification-results.json').write_text(json.dumps(results, indent=2) + '\n')
print('Result:', sum(r['success'] for r in results), '/', len(results), flush=True)
raise SystemExit(0 if all(r['success'] for r in results) else 1)
