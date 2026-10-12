"""Execute each emitted DLL in a separate Python process using explicit ABI tests."""
import ctypes as c
import json
import pathlib
import subprocess
import sys

root = pathlib.Path(__file__).resolve().parents[2]
out = root / 'build/vs2022/qualification'
# Each tuple: export, result types, input types, input values, expected values.
cases = {
 'return_42': [('answer','i','',[],[42])],
 'return_arithmetic_no_calls_valid': [('f','ii','i',[11],[12,22])],
 'return_trailing_zero_valid': [('partial','idci','i',[11],[16,3.14,0,0]),('empty','idc','',[],[0,0,0])],
 'rev33_constant': [],
 'rev36_call': [('answer','i','',[],[42]),('caller','i','',[],[42])],
 'rev37_arg': [('fortyTwo','i','i',[11],[42]),('caller','i','',[],[42])],
 'rev38_parameter': [('identity','i','i',[11],[11]),('caller','i','',[],[37])],
 'rev45_sqrt_literal': [('root','d','',[],[9.0])],
 'rev49_real_identity_add': [('identity','d','d',[1.5],[1.5]),('plus','d','',[],[3.75])],
 'rev49_real_literal': [('constant','d','',[],[3.5])],
 'rev49_real_parameter_add': [('addOffset','d','d',[1.5],[3.75]),('addReversed','d','d',[1.5],[3.75])],
 'rev49_real_parameter_subtract': [('subtractOffset','d','d',[1.5],[-0.75])],
 'rev51_multi_caller': [('three','idi','',[],[12,3.5,7]),('caller','i','',[],[12])],
 'rev51_multi_char_caller': [('three_char','ici','',[],[12,0,0]),('caller_char','c','',[],[0])],
 'rev51_multi_constant_arithmetic': [('computed','ii','',[],[17,20])],
 'rev51_multi_constant_multiply': [('computed','ii','',[],[20,30])],
 'rev51_multi_literal': [('three','idi','',[],[12,3.5,7])],
 'rev51_multi_parameter': [('replicate','idi','i',[11],[11,3.5,11])],
 'rev51_multi_real_caller': [('three','idi','',[],[12,3.5,7]),('caller_real','d','',[],[3.5])],
 'rev51_multi_runtime_add': [('computed','idi','i',[11],[11,3.5,21])],
 'rev51_multi_runtime_multiply': [('computed','iii','i',[11],[66,39,7])],
 'rev51_multi_runtime_sub': [('reduced','idi','i',[11],[11,3.5,1])],
 'rev51_multi_runtime_two_expressions': [('computed','idii','i',[11],[11,3.5,21,4])],
 'rev51_nested_arithmetic': [('computed','ii','i',[11],[26,2])],
 'rev51_real_parameter_multi_return': [('replicate_real','did','d',[1.5],[1.5,7,1.5])],
 'rev52_commutative_runtime_return': [('computed','iii','i',[11],[18,39,53])],
 'rev54_codeview_probe': [('answer','i','',[],[42])],
}
types = {'i': c.c_int32, 'd': c.c_double, 'c': c.c_uint8}
def execute(dll, source):
    library = c.CDLL(str(dll))
    library.main.argtypes = []
    library.main.restype = None
    library.main()
    checks = []
    for name, outputs, inputs, values, expected in cases[pathlib.Path(source).stem]:
        function = getattr(library, name)
        if len(outputs) == 1:
            function.restype = types[outputs]
            function.argtypes = [types[t] for t in inputs]
            actual = [function(*values)]
        else:
            class Result(c.Structure):
                _fields_ = [('f' + str(i), types[t]) for i, t in enumerate(outputs)]
            buffer = Result()
            c.memset(c.byref(buffer), 0xCC, c.sizeof(buffer))
            function.restype = None
            function.argtypes = [c.POINTER(Result)] + [types[t] for t in inputs]
            function(c.byref(buffer), *values)
            actual = [getattr(buffer, 'f' + str(i)) for i in range(len(outputs))]
        checks.append(dict(export=name, inputs=values, actual=actual, expected=expected, success=actual == expected))
    return checks

if len(sys.argv) > 1:
    print(json.dumps(execute(pathlib.Path(sys.argv[1]), sys.argv[2])))
    sys.exit(0)

results = []
for link in json.loads((out / 'link-results.json').read_text()):
    if not link['success']: continue
    proc = subprocess.run([sys.executable, str(pathlib.Path(__file__).resolve()), str(root / link['dll']), link['source']],
                          capture_output=True, text=True, timeout=30)
    checks = json.loads(proc.stdout) if proc.returncode == 0 else []
    row = dict(configuration=link['configuration'], mode=link['mode'], source=link['source'],
        exit_code=proc.returncode, checks=checks, success=proc.returncode == 0 and all(x['success'] for x in checks),
        main_smoke_pass=proc.returncode == 0, diagnostic=proc.stderr)
    results.append(row)
    if not row['success']: print('FAIL', row, flush=True)
(out / 'native-runtime-results.json').write_text(json.dumps(results, indent=2) + '\n')
print(f"DLL runtime cases: {sum(r['success'] for r in results)}/{len(results)}; explicit result assertions: {sum(len(r['checks']) for r in results)}")
