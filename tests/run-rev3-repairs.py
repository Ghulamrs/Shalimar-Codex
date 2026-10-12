"""Cross-target CLI regressions; no external target tools are required."""
import argparse
import json
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--compiler', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
compiler = str(Path(args.compiler).resolve())
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)
source = out / 'fixture.shl'
source.write_text('fun <int> = answer() { return 2 + 3 * 4 }\nfun main() {}\n')
results = []
def check(label, arguments, success, text=''):
    p = subprocess.run([compiler, str(source), *map(str, arguments)], capture_output=True, text=True, timeout=20)
    passed = (p.returncode == 0) == success and text in p.stdout + p.stderr
    results.append(dict(case=label, success=passed, exit=p.returncode, stdout=p.stdout, stderr=p.stderr))
    if not passed:
        raise AssertionError(f'{label}: {p.returncode}\n{p.stdout}\n{p.stderr}')
for mode in ('debug', 'release'):
    for target in ('windows-x86_64', 'linux-x86_64', 'macos-arm64', 'tms6747'):
        check(f'{target}-{mode}-frontend', [f'--target={target}', '--check', f'--{mode}'], True,
              'not target code-generation')
    for target in ('linux-x86_64', 'macos-arm64'):
        check(f'{target}-{mode}-native', [f'--target={target}', '--object', out/'unsupported.obj', f'--{mode}'], False,
              f'native output for {target} is not implemented')
    for flag in ('--debug-map', '--debug-variables', '--debug-live-ranges'):
        check(f'dsp-{mode}-{flag}', ['--target=tms6747', '--object', out/'unsupported.obj', flag, out/'report.tsv', f'--{mode}'],
              False, 'Windows x64 reports')
    check(f'dsp-{mode}-index-object', ['--target=tms6747', '--object', out/'unsupported.obj', '--dsp-debug-index', out/'index.tsv'],
          False, '--dsp-debug-index requires -S')
check('dsp-release-report', ['--target=tms6747', '-S', '-o', out/'constant.asm', '--release', '--optimization-report'],
      True, '[TMS6747/release] IR instructions: 3 -> 1')
check('symbols-check', ['--check', '--debug-symbols'], False, '--debug-symbols requires native output')
for mode in ('debug', 'release'):
    for form in ('return 2147483647 + 1', 'int x : 2147483647 return x + 1'):
        source.write_text(f'fun <int> = answer() {{ {form} }}\nfun main() {{}}\n')
        check(f'windows-{mode}-overflow-{form}', ['--object', out/'overflow.obj', f'--{mode}'], False, 'integer overflow')
        check(f'dsp-{mode}-overflow-{form}', ['--target=tms6747', '-S', '-o', out/'overflow.asm', f'--{mode}'], False, 'integer overflow')
(out/'results.json').write_text(json.dumps(results, indent=2))
print(f'{len(results)}/{len(results)} cross-target CLI contracts passed')
