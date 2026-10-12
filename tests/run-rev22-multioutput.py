#!/usr/bin/env python3
"""Regression contract for multi-output backend gap; exits nonzero on regressions."""
import pathlib, subprocess, tempfile, sys
root = pathlib.Path(__file__).resolve().parents[1]
compiler = pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else root/'build'/'shalimarc'
fixture = root/'tests'/'multi_output.shl'
with tempfile.TemporaryDirectory() as d:
    ast = pathlib.Path(d)/'ast.c'
    ir = pathlib.Path(d)/'ir.c'
    exe = pathlib.Path(d)/'program'
    def call(args):
        return subprocess.run([str(x) for x in args], capture_output=True, text=True, timeout=30)
    a = call([compiler,fixture,'--emit-c',ast])
    assert a.returncode == 0, a.stderr
    b = call(['cc','-std=c99',ast,'-lm','-o',exe])
    assert b.returncode == 0, b.stderr
    c = call([exe]); assert c.returncode == 0 and c.stdout.strip() == '3 2', repr(c.stdout)
    r = call([compiler,fixture,'--emit-c',ir,'--ir-native'])
    if r.returncode == 0:
        b = call(['cc','-std=c99',ir,'-lm','-o',exe]); assert b.returncode == 0, b.stderr
        c = call([exe]); assert c.returncode == 0 and c.stdout.strip() == '3 2', repr(c.stdout)
        print('PASS: multi-output AST and IR-native runtime parity')
    else:
        assert 'multi-output function ABI and return lowering are not implemented' in r.stderr, r.stderr
        print('KNOWN GAP: IR-native multi-output ABI not implemented; AST runtime PASS')
