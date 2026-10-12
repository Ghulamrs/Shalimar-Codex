"""Run from a Visual Studio x64 developer environment."""
import json
import pathlib
import subprocess

root = pathlib.Path(__file__).resolve().parents[2]
out = root / 'build/vs2022/qualification/cpp'
results = []
for configuration in ('Debug', 'Release'):
    folder = out / configuration
    folder.mkdir(parents=True, exist_ok=True)
    objects = [str(p) for p in (root / 'build/vs2022/shalimarc.dir' / configuration).glob('*.obj') if p.name != 'main.obj']
    assert len(objects) == 21, 'Expected the current compiler object files'
    for source in sorted((root / 'tests').glob('*.cpp')):
        exe = folder / (source.stem + '.exe')
        args = ['cl.exe', '/nologo', '/EHsc', '/std:c++14', '/UNDEBUG',
                '/MDd' if configuration == 'Debug' else '/MD',
                '/Od' if configuration == 'Debug' else '/O2',
                '/I' + str(root / 'include'), '/Fe' + str(exe),
                '/Fo' + str(folder / (source.stem + '.obj')), '/Fd' + str(folder / (source.stem + '.pdb')),
                str(source), '/link'] + objects
        build = subprocess.run(args, capture_output=True, text=True, timeout=60)
        run = None
        if build.returncode == 0:
            run_args = [str(exe)]
            if source.stem == 'rev35_coff_relocations': run_args.append(str(folder / 'relocations.obj'))
            run = subprocess.run(run_args, capture_output=True, text=True, timeout=30, cwd=folder)
        log = build.stdout + build.stderr + (run.stdout + run.stderr if run else '')
        (folder / (source.stem + '.log')).write_text(log)
        row = dict(configuration=configuration, source=source.relative_to(root).as_posix(),
            build_exit=build.returncode, run_exit=run.returncode if run else None,
            success=build.returncode == 0 and run.returncode == 0, diagnostic=log)
        results.append(row)
        print(configuration, source.name, 'PASS' if row['success'] else 'FAIL', flush=True)
(out.parent / 'cpp-results.json').write_text(json.dumps(results, indent=2) + '\n')
print(f"C++ tests: {sum(r['success'] for r in results)}/{len(results)}; assertions enabled in both builds")
