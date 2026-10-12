import json
import argparse
import pathlib
import subprocess
root = pathlib.Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--compiler-build', default='build/vs2022')
parser.add_argument('--toolchain', choices=('ti', 'shalimar'), default='ti')
parser.add_argument('--tools-root', default='C:/Users/GRA/source')
args = parser.parse_args()
out = root / ('build/tms6747-corpus' if args.toolchain == 'ti' else 'build/tms6747-own-corpus')
out.mkdir(parents=True, exist_ok=True)
sources = sorted(p for folder in ('examples', 'tests') for p in (root / folder).rglob('*') if p.suffix in ('.shl', '.shm'))
results = []
for configuration in ('Debug', 'Release'):
    compiler = root / args.compiler_build / configuration / 'shalimarc.exe'
    for mode in ('debug', 'release'):
        print(configuration, mode, 'probing', len(sources), 'sources', flush=True)
        for source in sources:
            relative = source.relative_to(root)
            target = out / configuration / mode / relative.with_suffix('.obj')
            target.parent.mkdir(parents=True, exist_ok=True)
            process = subprocess.run([str(compiler), source.as_posix(), '--object', str(target),
                                      '--target=tms6747', '--toolchain=' + args.toolchain,
                                      '--tools-root', args.tools_root, '--' + mode], capture_output=True, text=True, timeout=120, cwd=root)
            diagnostic = process.stdout + process.stderr
            target.with_suffix('.log').write_text(diagnostic, encoding='utf-8')
            results.append(dict(configuration=configuration, mode=mode, source=relative.as_posix(),
                success=process.returncode == 0 and target.exists() and target.stat().st_size > 0,
                exit_code=process.returncode, diagnostic=diagnostic))
report = dict(source_count=len(sources), toolchain=args.toolchain, results=results)
(out / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
for configuration in ('Debug', 'Release'):
    for mode in ('debug', 'release'):
        rows = [r for r in results if r['configuration'] == configuration and r['mode'] == mode]
        print(configuration, mode, sum(r['success'] for r in rows), '/', len(rows), flush=True)
