import json
import pathlib
import subprocess

root = pathlib.Path(__file__).resolve().parents[2]
out = root / 'build/vs2022/qualification/symbols'
out.mkdir(exist_ok=True)
tools = pathlib.Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64')
results = []
for configuration in ('Debug', 'Release'):
    for mode in ('debug', 'release'):
        name = configuration + '-' + mode
        obj = out / (name + '.obj')
        dll = out / (name + '.dll')
        pdb = out / (name + '.pdb')
        emit = subprocess.run([str(root / 'build/vs2022' / configuration / 'shalimarc.exe'),
            str(root / 'tests/rev54_codeview_probe.shl'), '--object', str(obj), '--' + mode, '--debug-symbols'], capture_output=True, text=True)
        headers = subprocess.run([str(tools / 'dumpbin.exe'), '/headers', str(obj)], capture_output=True, text=True)
        link = subprocess.run([str(tools / 'link.exe'), '/nologo', '/dll', '/noentry', '/nodefaultlib',
            '/debug', '/export:answer', '/out:' + str(dll), '/pdb:' + str(pdb), str(obj)], capture_output=True, text=True)
        log = emit.stdout + emit.stderr + headers.stdout + headers.stderr + link.stdout + link.stderr
        (out / (name + '.log')).write_text(log)
        results.append(dict(configuration=configuration, mode=mode, emit_exit=emit.returncode,
            codeview_section='.debug$S' in headers.stdout, link_exit=link.returncode,
            pdb_generated=pdb.exists() and pdb.stat().st_size > 0,
            debugger_stepping='NOT TESTED'))
(out.parent / 'symbols-results.json').write_text(json.dumps(results, indent=2) + '\n')
print(json.dumps(results, indent=2))
