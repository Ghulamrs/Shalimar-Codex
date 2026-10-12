#!/usr/bin/env python3
"""Compile the unchanged supplied programs and compare their expected comments."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
NUMBER = re.compile(r'^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?$')


def matches(actual, expected):
    left, right = actual.split(), expected.split()
    if len(left) != len(right):
        return False
    for observed, wanted in zip(left, right):
        if NUMBER.fullmatch(observed) and NUMBER.fullmatch(wanted):
            if '.' in wanted or 'e' in wanted.lower():
                places = len(wanted.split('.')[1]) if '.' in wanted and 'e' not in wanted.lower() else 7
                tolerance = 0.500001 * 10 ** -places
                if not math.isclose(float(observed), float(wanted), rel_tol=0, abs_tol=tolerance):
                    return False
            elif float(observed) != float(wanted):
                return False
        elif observed != wanted:
            return False
    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--compiler', required=True)
    parser.add_argument('--mode', choices=['debug', 'release'], default='debug')
    parser.add_argument('--backend', choices=['ast', 'ir'], default='ast')
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    compiler = str(Path(args.compiler).resolve())
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    results = []
    for source in sorted((ROOT / 'tests/original').glob('*.shm')):
        record = {'program': source.name, 'sha256': hashlib.sha256(source.read_bytes()).hexdigest()}
        c_path, executable = output / (source.stem + '.c'), output / source.stem
        commands = [
            [compiler, str(source), '--emit-c', str(c_path), '--' + args.mode] + (['--ir-native'] if args.backend == 'ir' else []),
            ['cc', '-std=c99', '-O2' if args.mode == 'release' else '-O0', str(c_path), '-lm', '-o', str(executable)],
            [str(executable)],
        ]
        try:
            for stage, command in zip(['compile', 'native-build', 'execute'], commands):
                run = subprocess.run(command, capture_output=True, text=True, timeout=30)
                if run.returncode:
                    record.update(status='FAIL', stage=stage, returncode=run.returncode, diagnostic=run.stderr)
                    break
            else:
                (output / (source.stem + '.out')).write_text(run.stdout)
                expected = re.findall(r'^// expected: ?(.*)$', source.read_text(), re.MULTILINE)
                actual = run.stdout.splitlines()
                position = 0
                for line in expected:
                    while position < len(actual) and not matches(actual[position], line):
                        position += 1
                    if position == len(actual):
                        record.update(status='FAIL', stage='compare', expected_line=line, actual=run.stdout)
                        break
                    position += 1
                else:
                    # table.shm supplies only selected rows. Verify every row independently.
                    if source.stem == 'table':
                        all_expected = ['table of 7'] + [f'7 x {i} = {7*i}' for i in range(1, 11)]
                        if len(actual) != len(all_expected) or not all(matches(a, e) for a, e in zip(actual, all_expected)):
                            record.update(status='FAIL', stage='table-oracle', actual=run.stdout)
                        else:
                            record.update(status='PASS', expected_lines=len(expected), verified_lines=len(actual))
                    elif len(actual) != len(expected):
                        record.update(status='FAIL', stage='unexpected-output-lines', actual=run.stdout)
                    else:
                        record.update(status='PASS', expected_lines=len(expected), verified_lines=len(actual))
        except subprocess.TimeoutExpired:
            record.update(status='FAIL', stage=stage, diagnostic='timeout')
        results.append(record)
        print(source.name + ': ' + record['status'])
    report = {'mode': args.mode, 'backend': args.backend, 'passed': sum(r['status'] == 'PASS' for r in results), 'total': len(results), 'results': results}
    (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] == report['total'] == 12 else 1


if __name__ == '__main__':
    sys.exit(main())
