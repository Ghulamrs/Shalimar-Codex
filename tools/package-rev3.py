import hashlib
import json
import pathlib
import zipfile
root = pathlib.Path(__file__).resolve().parents[1]
destination = root.parents[2] / 'Shalimar_TMS6747_Windows_Merge_Rev3.zip'
files = sorted(p for p in root.rglob('*') if p.is_file() and '__pycache__' not in p.parts and '.git' not in p.parts and p.name != 'ti-assembler-reference.txt')
manifest = {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in files
    if p.suffix in ('.cpp', '.h', '.py', '.ps1', '.js', '.shl', '.shm', '.vcxproj', '.sln') and 'build' not in p.relative_to(root).parts and 'x64' not in p.relative_to(root).parts}
path = root / 'docs/REV3_SOURCE_MANIFEST.json'
path.write_text(json.dumps(manifest, indent=2) + '\n')
if path not in files: files.append(path)
with zipfile.ZipFile(destination, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
    for file in files:
        archive.write(file, 'Shalimar_TMS6747_Windows_Merge_Rev3/ShalimarCompiler/' + file.relative_to(root).as_posix())
with zipfile.ZipFile(destination) as archive:
    assert archive.testzip() is None
    print('Verified', len(archive.namelist()), 'files,', destination.stat().st_size, 'bytes')
print(destination)
