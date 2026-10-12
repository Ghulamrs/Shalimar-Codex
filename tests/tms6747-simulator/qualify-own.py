"""Keep our tools separate; CCS 5.5 is the independent machine-code oracle."""
import argparse
import json
import pathlib
import struct
import subprocess
parser = argparse.ArgumentParser()
parser.add_argument('--compiler-build', default='build/vs2022')
parser.add_argument('--tools-root', default=r'C:/Users/GRA/source')
parser.add_argument('--rts-lib', default=r'C:/Users/GRA/Developer/Claude/M10/RTS6x/build/rts6x.lib')
parser.add_argument('--quick', action='store_true')
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parents[2]
out = root / 'build/tms6747-own'
out.mkdir(parents=True, exist_ok=True)
tools = pathlib.Path(args.tools_root)
def tool(name):
    return str(tools / 'bin' / name if (tools / 'bin' / name).exists() else tools / name)
def run(command, log):
    result = subprocess.run(command, capture_output=True, text=True, timeout=120, cwd=root)
    text = result.stdout + result.stderr
    log.write_text(text, encoding='utf-8')
    return result.returncode, text
cases = [('return_42', 42, 'int'), ('scalar_calls', 42, 'int'), ('scalar_loop', 42, 'int'),
    ('scalar_multiply', 42, 'int'), ('real_arithmetic', -0.75, 'real'), ('mixed_real', 3.5, 'real'),
    ('ten_args', 55, 'int'), ('ten_real_args', 60, 'real'), ('recursion', 720, 'int'),
    ('nested_calls', 42, 'int'), ('signed_comparisons', 42, 'int'), ('real_multiply', 3.375, 'real')]
results = []
for configuration in (('Debug',) if args.quick else ('Debug', 'Release')):
    for mode in ('debug', 'release'):
        compiler = root / args.compiler_build / configuration / 'shalimarc.exe'
        for name, expected, kind in cases:
            label = configuration + '-' + mode + '-' + name
            executable = out / (label + '.out')
            source = root / 'tests/tms6747' / (name + '.shl')
            code, diagnostic = run([str(compiler), str(source), '--target=tms6747', '--toolchain=shalimar',
                '--tools-root', args.tools_root, '--rts-lib', args.rts_lib, '--' + mode,
                '--dsp-entry', 'answer', '-o', str(executable)], out / (label + '.build.log'))
            vm_ok = machine_ok = ccs_ok = False
            if code == 0:
                # The assembly VM's main returns an exact word-comparison verdict,
                # including both words of real results. No host runtime call computes it.
                low, high = struct.unpack('<II', struct.pack('<d', expected)) if kind == 'real' else (int(expected) & 0xffffffff, 0)
                probe = out / (label + '.vm-probe.s')
                body = '\tMVKL ' + str(low) + ', A0\n\tMVKH ' + str(low) + ', A0\n\tCMPEQ A4, A0, A0\n'
                if kind == 'real':
                    body += '\tMVKL ' + str(high) + ', A1\n\tMVKH ' + str(high) + ', A1\n\tCMPEQ A5, A1, A1\n\tAND A0, A1, A0\n'
                body += '\tXOR 1, A0, A4\n'
                probe.write_text('\t.global main\n\t.text\nmain:\n\tSTW B3, *B15--(8)\n'
                    '\tMVKL probe_return, B3\n\tMVKH probe_return, B3\n\tB answer\n\tNOP 5\nprobe_return:\n' + body +
                    '\tLDW *++B15(8), B3\n\tNOP 4\n\tB B3\n\tNOP 5\n')
                vmcode, _ = run([tool('vm6747.exe'), str(executable) + '.asm', str(probe), '-c'], out / (label + '.vm.log'))
                vm_ok = vmcode == 0
                machinecode, machine = run([tool('sim6747.exe'), '--run', str(executable), '--main-status', '-c'], out / (label + '.machine.log'))
                # Full real values are independently checked in VM and CCS;
                # machine simulator uses the generated wrapper's integer status.
                status = int(expected) if kind == 'int' else 0
                machine_ok = machinecode == (status & 255) and 'exit=C$$EXIT status=' + str(status) in machine
                ccscode, diagnostic = run([r'C:/ti/ccsv5/ccs_base/scripting/bin/dss.bat',
                    str(root / 'tests/tms6747-simulator/run-shalimar.js'), root.as_posix(), executable.as_posix(),
                    str(expected), kind], out / (label + '.ccs.log'))
                ccs_ok = ccscode == 0 and 'PASS: Shalimar-generated DSP function' in diagnostic and 'FAIL' not in diagnostic
            row = dict(configuration=configuration, mode=mode, case=name, expected=expected, kind=kind,
                build_exit=code, vm_ok=vm_ok, machine_ok=machine_ok, ccs_ok=ccs_ok,
                success=code == 0 and vm_ok and machine_ok and ccs_ok, diagnostic=diagnostic)
            results.append(row)
            print(label, 'PASS' if row['success'] else 'FAIL', 'VM=' + str(vm_ok), 'machine=' + str(machine_ok), 'CCS=' + str(ccs_ok), flush=True)
(out / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
print('Own toolchain:', sum(r['success'] for r in results), '/', len(results), flush=True)
raise SystemExit(0 if all(r['success'] for r in results) else 1)
