"""Driver contract tests; invoke outside the sandbox for the installed TI tools."""
import json
import argparse
import pathlib
import subprocess
root = pathlib.Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--compiler-build', default='build/vs2022')
args = parser.parse_args()
out = root / 'build/tms6747-driver/path with spaces'
out.mkdir(parents=True, exist_ok=True)
source = root / 'tests/tms6747/scalar_calls.shl'
results = []
for configuration in ('Debug', 'Release'):
    compiler = root / args.compiler_build / configuration / 'shalimarc.exe'
    for mode in ('debug', 'release'):
        prefix = configuration + '-' + mode
        assembly, obj, exe = (out / (prefix + suffix) for suffix in ('.asm', '.obj', '.out'))
        cases = [
            ('assembly', ['-S', str(assembly), '--target=tms6747'], 0, assembly, b'\t.text'),
            ('object', ['--target=tms6747', '--object', str(obj)], 0, obj, b'\x7fELF'),
            ('executable', ['--target=tms6747', '--dsp-entry', 'answer', '-o', str(exe)], 0, exe, b'\x7fELF'),
            ('unknown-target', ['--check', '--target=unknown'], 2, None, b'Unknown argument'),
            ('missing-output', ['--target=tms6747', '--executable'], 2, None, b'output path required'),
            ('missing-entry', ['--target=tms6747', '--dsp-entry', 'absent', '-o', str(out / (prefix + '-missing.out'))], 1, None, b'entry function not found'),
            ('parameter-entry', ['--target=tms6747', '--dsp-entry', 'plus', '-o', str(out / (prefix + '-parameter.out'))], 1, None, b'entry must take no arguments'),
            ('missing-toolchain', ['--target=tms6747', '--ti-root', str(out / 'nonexistent-toolchain'), '--dsp-entry', 'answer', '-o', str(out / (prefix + '-missing-tool.out'))], 1, None, b'Cannot launch target tool'),
        ]
        for name, arguments, expected, artifact, marker in cases:
            process = subprocess.run([str(compiler), str(source), '--toolchain=ti', '--' + mode] + arguments,
                capture_output=True, timeout=120, cwd=root)
            diagnostic = process.stdout + process.stderr
            data = artifact.read_bytes() if artifact and artifact.exists() else diagnostic
            success = process.returncode == expected and marker in data
            if name == 'assembly':
                success = success and (b'.dwtag' in data) == (mode == 'debug')
            label = prefix + '-' + name
            (out / (label + '.log')).write_bytes(diagnostic)
            results.append(dict(configuration=configuration, mode=mode, case=name,
                exit_code=process.returncode, expected_exit=expected, success=success,
                diagnostic=diagnostic.decode(errors='replace')))
            print(label, 'PASS' if success else 'FAIL', flush=True)
(out.parent / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
raise SystemExit(0 if all(r['success'] for r in results) else 1)
