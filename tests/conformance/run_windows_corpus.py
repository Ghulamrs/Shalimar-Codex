"""Probe every supplied Shalimar source; preserve diagnostics and artifacts."""
import collections
import hashlib
import json
import pathlib
import subprocess

root = pathlib.Path(__file__).resolve().parents[2]
out = root / 'build/vs2022/qualification'
out.mkdir(parents=True, exist_ok=True)
sources = sorted(p for folder in ('examples', 'tests') for p in (root / folder).rglob('*')
                 if p.suffix.lower() in ('.shl', '.shm'))
negative_names = set('''array_initializer_too_many array_initializer_bad_shape
array_literal_assignment_too_many global_array_reassignment_too_many
semantic_error scalar_reference_reject_literal global_later_reference
global_initializer_later_reference library_missing_borrow library_bad_arity
library_bad_name library_name_collision prec_bad_arity prec_bad_type
rev13_invalid_value_return return_no_nested_calls_invalid return_excess_invalid
return_direct_call_invalid'''.split())
# These historical positive fixtures use array return types prohibited by 3.0.
array_return_names = {'string_return', 'ir_native_global_string_initializers', 'rev14_char_return_contract'}
results = []
def invoke(compiler, source, mode, stage, target):
    arguments = [str(compiler), str(source)]
    arguments += ['--object', str(target)] if stage == 'object' else ['--' + stage]
    arguments += ['--' + mode]
    try:
        proc = subprocess.run(arguments, capture_output=True, text=True, timeout=30)
        log = proc.stdout + proc.stderr
        code = proc.returncode
    except subprocess.TimeoutExpired:
        code, log = 'TIMEOUT', 'Compiler exceeded 30 second timeout.'
    target.with_suffix('.log').write_text(log, encoding='utf-8')
    return code, log

for configuration in ('Debug', 'Release'):
    compiler = root / 'build/vs2022' / configuration / 'shalimarc.exe'
    for mode in ('debug', 'release'):
        print(f'Running {configuration} compiler, --{mode}, {len(sources)} sources', flush=True)
        for source in sources:
            relative = source.relative_to(root).as_posix()
            expected = 'reject' if source.stem in negative_names | array_return_names else 'unclassified'
            for stage in ('check', 'dump-ir', 'object'):
                target = out / configuration / mode / stage / (relative + '.obj')
                target.parent.mkdir(parents=True, exist_ok=True)
                code, diagnostic = invoke(compiler, source, mode, stage, target)
                success = code == 0 and (stage != 'object' or target.exists() and target.stat().st_size > 0)
                results.append(dict(configuration=configuration, mode=mode, source=relative,
                    stage=stage, expected_frontend=expected, exit_code=code, success=success,
                    diagnostic=diagnostic, artifact=str(target.relative_to(root)) if success and stage == 'object' else None))
    # Explicit normative negative cases, independent of filename assumptions.
    for case in json.loads((root / 'tests/conformance/negative_cases.json').read_text()):
        source = out / 'negative-sources' / (case['id'] + '.shl')
        source.parent.mkdir(exist_ok=True)
        source.write_text(case['source'] + '\n')
        for mode in ('debug', 'release'):
            target = out / configuration / mode / 'negative' / (case['id'] + '.obj')
            target.parent.mkdir(parents=True, exist_ok=True)
            code, diagnostic = invoke(compiler, source, mode, 'check', target)
            results.append(dict(configuration=configuration, mode=mode, source='negative:' + case['id'],
                stage='negative', expected_frontend='reject', exit_code=code,
                success=code == 1 and any(marker in diagnostic for marker in ('lex error:', 'parse error:', 'semantic error:')), diagnostic=diagnostic))

summary = []
for configuration in ('Debug', 'Release'):
    for mode in ('debug', 'release'):
        for stage in ('check', 'dump-ir', 'object', 'negative'):
            rows = [r for r in results if (r['configuration'], r['mode'], r['stage']) == (configuration, mode, stage)]
            summary.append(dict(configuration=configuration, mode=mode, stage=stage,
                                total=len(rows), successful=sum(r['success'] for r in rows)))
report = dict(date='2026-10-11', source_count=len(sources),
    source_manifest={p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
    specification_sha256=hashlib.sha256((root / 'docs/SHALIMAR_LANGUAGE.md').read_bytes()).hexdigest(),
    method='All supplied .shl/.shm sources; both compiler builds crossed with both source modes; success is stage completion, not conformance.',
    summary=summary, results=results)
(out / 'corpus-results.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
import csv
with (out / 'corpus-results.csv').open('w', newline='', encoding='utf-8') as stream:
    fields = ['configuration', 'mode', 'source', 'stage', 'expected_frontend', 'exit_code', 'success', 'diagnostic', 'artifact']
    writer = csv.DictWriter(stream, fieldnames=fields)
    writer.writeheader()
    writer.writerows(results)
print(json.dumps(summary, indent=2))
print('Report:', out / 'corpus-results.json')
