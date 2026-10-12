import argparse
import ctypes
import json
import subprocess
import sys
from pathlib import Path

if len(sys.argv)>1 and sys.argv[1]=='--child':
    dll=ctypes.CDLL(sys.argv[2]); dll.compute.argtypes=[ctypes.c_int]; dll.compute.restype=ctypes.c_int
    print(dll.compute(int(sys.argv[3])),flush=True); raise SystemExit(0)
parser=argparse.ArgumentParser()
parser.add_argument('--configuration',choices=['Debug','Release'])
parser.add_argument('--compiler-build', required=True)
parser.add_argument('--output', required=True)
args=parser.parse_args()
base=Path(args.compiler_build).resolve()
root=Path(__file__).resolve().parents[1]
tools=root/'toolsets/shalimar-c6747'
out=Path(args.output).resolve(); out.mkdir(parents=True,exist_ok=True)
link=Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/link.exe')
kernel=Path('C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/kernel32.lib')
cases=[
 ('first','int A[3] : {10,20,30} return A[i]',0,10,None),
 ('last','int A[3] : {10,20,30} return A[i]',2,30,None),
 ('zero','int A[3] return A[i]',2,0,None),
 ('omitted','int A[] : {10,,30} return A[i]',1,0,None),
 ('default','int A[] return A[i]',2,0,None),
 ('write','int A[3] A[i] : 42 return A[i]',1,42,None),
 ('computed','int A[3] A[i+1] : i*2 return A[i+1]',1,2,None),
 ('copy','int A[3] int B[3] : {10,20,30} A : B return A[i]',1,20,None),
 ('negative','int A[3] return A[i]',-1,None,'array index outside range [0, 2]'),
 ('upper','int A[3] return A[i]',3,None,'array index outside range [0, 2]'),
 ('invalid_store','int A[3] A[i] : 42 return 0',3,None,'array index outside range [0, 2]'),
 ('bounds_before_value','int A[3] A[i] : i+2147483647 return 0',3,None,'array index outside range [0, 2]'),
 ('valid_index_value_overflow','int A[3] A[i] : i+2147483647 return 0',1,None,'integer overflow'),
 ('large_negative','int A[3] return A[i]',-2147483648,None,'array index outside range [0, 2]'),
 ('rows','int A[3] return A.row',0,3,None),
 ('columns','int A[3] return A.col',0,-1,None),
 ('axis_zero','int A[3] return A.dim(i)',0,3,None),
 ('axis_missing','int A[3] return A.dim(i)',2,-1,None),
 ('axis_negative','int A[3] return A.dim(i)',-1,-1,None),
 ('length','int A[3] return len(A)',0,3,None),
 ('copy_independence','int A[3] int B[3] : {10,20,30} A : B B[i] : 42 return A[i]',1,20,None),
 ('self_copy','int A[3] : {10,20,30} A : A return A[i]',2,30,None),
 ('computed_initializer','int A[3] : {i,i+1,i*2} return A[2]',3,6,None),
 ('unused_read_traps','int A[3] int x : A[i] return 0',3,None,'array index outside range [0, 2]'),
]
def run(command):
    return subprocess.run(list(map(str,command)),capture_output=True,text=True,timeout=40)
results=[]
for config in ([args.configuration] if args.configuration else ['Debug','Release']):
 for mode in ['debug','release']:
  for name,body,index,value,reason in cases:
   for target in ['windows-x86_64','tms6747']:
    stem=f'{config}-{mode}-{target}-{name}'; source=out/(stem+'.shl')
    source.write_text(('uses len\n' if 'len(' in body else '')+f'fun <int> = compute(i: int) {{\n{body}\n}}\n'+
                      (f'fun <int> = answer() {{ return compute({index}) }}\n' if target=='tms6747' else '')+'fun main() {}\n')
    compiler=base/config/'shalimarc.exe'
    commands=[]
    if target=='windows-x86_64':
     obj=out/(stem+'.obj'); dll=out/(stem+'.dll')
     commands=[[compiler,source,'--object',obj,'--'+mode],
               [link,'/nologo','/dll','/noentry','/nodefaultlib','/export:compute','/out:'+str(dll),obj,base/config/'shalimar_runtime.lib',kernel]]
     execute=[sys.executable,Path(__file__),'--child',dll,index]
    else:
     exe=out/(stem+'.out')
     commands=[[compiler,source,'--target=tms6747','--tools-root',tools,'--rts-lib',tools/'lib/rts6x.lib','--dsp-entry','answer','--'+mode,'-o',exe]]
     execute=[tools/'bin/sim6747.exe','--run',exe,'--main-status','-c']
    logs=[]; built=True
    for command in commands:
     p=run(command); logs.append(p.stdout+p.stderr)
     if p.returncode: built=False; break
    p=run(execute) if built else None
    expected_exit=1 if reason else (0 if target=='windows-x86_64' else value&255)
    success=built and p.returncode==expected_exit
    if built:
     if reason: success=success and f'Error: line 2: {reason}\n' in p.stdout+p.stderr
     elif target=='windows-x86_64': success=success and p.stdout.strip()==str(value)
     else: success=success and f'exit=C$$EXIT status={value}' in p.stdout+p.stderr
    result=dict(case=name,configuration=config,mode=mode,target=target,success=success,build_logs=logs,
                exit=p.returncode if p else None,stdout=p.stdout if p else '',stderr=p.stderr if p else '')
    results.append(result)
    if not success: print('FAIL',stem,json.dumps(result),flush=True)
filename=out/f'results-{args.configuration or "all"}.json'
filename.write_text(json.dumps(results,indent=2))
print(f'Native int arrays: {sum(r["success"] for r in results)}/{len(results)}',flush=True)
raise SystemExit(0 if all(r['success'] for r in results) else 1)
