#!/usr/bin/env python3
"""Exact output and diagnostic checks for repaired normative behavior."""
import argparse
from pathlib import Path
import subprocess
import sys

EXPECTED = ('2 3 \nhello hello world 12 \n'
            '1.000000  -2.000000\n3.250000   4.500000 \n'
            '1.00  -2.00\n3.25   4.50 \n7 \n'
            '1.00 1.50 2.00 done \n0.00 1.00 end \n3.14 3 \n')
ROOT = Path(__file__).resolve().parent.parent


def run(command):
    return subprocess.run(command, capture_output=True, text=True, timeout=20)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--compiler', required=True)
    parser.add_argument('--mode', choices=['debug', 'release'], default='debug')
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    compiler = str(Path(args.compiler).resolve())
    folder = Path(args.output).resolve()
    folder.mkdir(parents=True, exist_ok=True)
    cfile, binary = folder / 'values.c', folder / 'values'
    p = run([compiler, str(ROOT / 'tests/conformance_values.shl'), '--emit-c', str(cfile), '--' + args.mode])
    assert p.returncode == 0, p.stderr
    p = run(['cc', '-std=c99', str(cfile), '-lm', '-o', str(binary)])
    assert p.returncode == 0, p.stderr
    p = run([str(binary)])
    assert p.returncode == 0 and p.stdout == EXPECTED, repr(p.stdout)
    (folder / 'values.out').write_text(p.stdout)
    invalid = {
        'inline-print': ('fun main() { ? 1 }\n', 'print command must start its line'),
        'second-print': ('fun main() {\n? 1 ?? 2\n}\n', 'expected expression'),
        'array-copy-creation': ('fun main() {\nint a[2]\nb : a\n}\n', "Declare the array 'b' first"),
        'scope': ('fun main() {\nif 1 { x : 2 }\n? x\n}\n', "unknown variable 'x'"),
    }
    for name, (source, message) in invalid.items():
        path = folder / (name + '.shl')
        path.write_text(source)
        p = run([compiler, str(path), '--check'])
        assert p.returncode != 0 and message in p.stderr, p.stderr
    zero = folder / 'zero-step.shl'
    zero.write_text('fun main() {\nfor i : 0 to 3 step 0 {\n? i\n}\n}\n')
    p = run([compiler, str(zero), '--emit-c', str(cfile), '--' + args.mode])
    assert p.returncode == 0, p.stderr
    p = run(['cc', '-std=c99', str(cfile), '-lm', '-o', str(binary)])
    assert p.returncode == 0, p.stderr
    p = run([str(binary)])
    assert p.returncode != 0 and 'for step must not be zero' in p.stderr, p.stderr
    inferred = folder / 'inferred-string.shl'
    inferred.write_text('fun main() {\ns : "abc"\n? s\n}\n')
    p = run([compiler, str(inferred), '--emit-c', str(cfile), '--ir-native'])
    assert p.returncode != 0 and 'inferred-capacity string' in p.stderr, p.stderr
    print('Exact conformance output, four rejection checks, zero-step diagnostic, and IR limitation diagnostic passed.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
