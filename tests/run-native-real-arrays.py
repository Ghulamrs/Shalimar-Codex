"""Local real-vector execution on Windows and DSP; exact DSP values use VM6747."""
import argparse, ctypes, json, struct, subprocess, sys
from pathlib import Path
if len(sys.argv)>1 and sys.argv[1]=='--child':
    dll=ctypes.CDLL(sys.argv[2]); dll.compute.argtypes=[ctypes.c_int]; dll.compute.restype=ctypes.c_double
    print(struct.pack('<d',dll.compute(int(sys.argv[3]))).hex(),flush=True); raise SystemExit(0)
parser=argparse.ArgumentParser();parser.add_argument('--compiler-build',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--configuration',choices=['Debug','Release']);args=parser.parse_args()
root=Path(__file__).resolve().parent.parent;build=args.compiler_build.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True);tools=root/'toolsets/shalimar-c6747'
link=Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/link.exe');kernel=Path('C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/kernel32.lib')
cases=[
('first','real A[3] : {1.25,2.5,3.75} return A[i]',0,1.25,None),
('last','real A[3] : {1.25,2.5,3.75} return A[i]',2,3.75,None),
('zero','real A[3] return A[i]',2,0.0,None),
('default','real A[] return A[i]',2,0.0,None),
('omitted','real A[] : {1.25,,3.75} return A[i]',1,0.0,None),
('write','real A[3] A[i] : 2.75 return A[i]',1,2.75,None),
('integer_store','real A[3] A[i] : i return A[i]',2,2.0,None),
('computed','real A[3] A[i+1] : i*2.5 return A[i+1]',1,2.5,None),
('computed_initializer','real A[3] : {i+0.25,i+1.5,i*2.5} return A[2]',3,7.5,None),
('add','real A[2] : {1.25,2.5} return A[i]+A[1]',0,3.75,None),
('subtract','real A[2] : {1.25,2.5} return A[i]-A[1]',0,-1.25,None),
('multiply','real A[2] : {1.25,2.5} return A[i]*A[1]',0,3.125,None),
('negative','real A[2] : {-1.25,2.5} return A[i]',0,-1.25,None),
('negate','real A[2] : {1.25,2.5} return -A[i]',0,-1.25,None),
('scalar','real A[2] : {1.25,2.5} real x : A[i] return x+1',0,2.25,None),
('copy','real A[2] real B[2] : {1.25,2.5} A : B return A[i]',1,2.5,None),
('copy_independence','real A[2] real B[2] : {1.25,2.5} A : B B[i] : 9.25 return A[i]',1,2.5,None),
('self_copy','real A[2] : {1.25,2.5} A : A return A[i]',1,2.5,None),
('mixed_arrays','int I[2] : {2,3} real A[2] : {1.25,2.5} return A[i]+I[i]',1,5.5,None),
('decimal','real A[2] : {0.1,0.2} return A[i]',0,0.1,None),
('negative_zero','real A[2] : {-0.,1.} return A[i]',0,-0.0,None),
('rows','real A[3] return A.row',0,3.0,None),
('columns','real A[3] return A.col',0,-1.0,None),
('axis','real A[3] return A.dim(i)',0,3.0,None),
('missing_axis','real A[3] return A.dim(i)',2,-1.0,None),
('len','real A[3] return len(A)',0,3.0,None),
('negative_index','real A[3] return A[i]',-1,None,'array index outside range [0, 2]'),
('upper_index','real A[3] return A[i]',3,None,'array index outside range [0, 2]'),
('invalid_store','real A[3] A[i] : 2.75 return 0.',3,None,'array index outside range [0, 2]'),
('bounds_before_value','real A[3] A[i] : i+2147483647 return 0.',3,None,'array index outside range [0, 2]'),
('value_overflow','real A[3] A[i] : i+2147483647 return 0.',1,None,'integer overflow'),
]
def run(cmd):return subprocess.run(list(map(str,cmd)),capture_output=True,text=True,timeout=40)
results=[]
for config in ([args.configuration] if args.configuration else ['Debug','Release']):
 for mode in ['debug','release']:
  for name,body,index,value,reason in cases:
   for target in ['windows-x86_64','tms6747']:
    stem=config+'-'+mode+'-'+target+'-'+name;source=out/(stem+'.shl')
    source.write_text(('uses len\n' if 'len(' in body else '')+'fun <real> = compute(i: int) {\n'+body+'\n}\n'+('fun <real> = answer() { return compute('+str(index)+') }\n' if target=='tms6747' else '')+'fun main() {}\n')
    compiler=build/config/'shalimarc.exe';obj=out/(stem+'.obj');dll=out/(stem+'.dll');exe=out/(stem+'.out')
    commands=([[compiler,source,'--object',obj,'--'+mode,'--debug-symbols'],[link,'/nologo','/dll','/noentry','/nodefaultlib','/export:compute','/out:'+str(dll),obj,build/config/'shalimar_runtime.lib',kernel]] if target=='windows-x86_64' else [[compiler,source,'--target=tms6747','--tools-root',tools,'--rts-lib',tools/'lib/rts6x.lib','--dsp-entry','answer','--'+mode,'-o',exe]])
    logs=[];built=True
    for command in commands:
     proc=run(command);logs.append(proc.stdout+proc.stderr)
     if proc.returncode:built=False;break
    proc=run([sys.executable,Path(__file__),'--child',dll,index] if target=='windows-x86_64' else [tools/'bin/sim6747.exe','--run',exe,'--main-status','-c']) if built else None
    vm=None
    success=built and proc.returncode==(1 if reason else 0)
    if built:
     text=proc.stdout+proc.stderr
     if reason:success=success and 'Error: line 2: '+reason+'\n' in text
     elif target=='windows-x86_64':success=success and proc.stdout.strip()==struct.pack('<d',value).hex()
     else:
      low,high=struct.unpack('<II',struct.pack('<d',value));probe=out/(stem+'.probe.s')
      probe.write_text('\t.global main\n\t.text\nmain:\n\tSTW B3, *B15--(8)\n\tMVKL probe_return, B3\n\tMVKH probe_return, B3\n\tB answer\n\tNOP 5\nprobe_return:\n'+f'\tMVKL {low}, A0\n\tMVKH {low}, A0\n\tCMPEQ A4, A0, A0\n\tMVKL {high}, A1\n\tMVKH {high}, A1\n\tCMPEQ A5, A1, A1\n\tAND A0, A1, A0\n\tXOR 1, A0, A4\n'+'\tLDW *++B15(8), B3\n\tNOP 4\n\tB B3\n\tNOP 5\n')
      vm=run([tools/'bin/vm6747.exe',str(exe)+'.asm',probe,'-c']);success=success and vm.returncode==0 and 'exit=C$$EXIT status=0' in text
    record=dict(case=name,host=config,mode=mode,target=target,passed=success,build_logs=logs,output=(proc.stdout+proc.stderr) if proc else '',vm_output=(vm.stdout+vm.stderr) if vm else '')
    results.append(record)
    if not success:print('FAIL',stem,json.dumps(record),flush=True)
(out/('results-'+(args.configuration or 'all')+'.json')).write_text(json.dumps(results,indent=2))
print(str(sum(r['passed'] for r in results))+'/'+str(len(results))+' real-array checks passed',flush=True)
raise SystemExit(0 if all(r['passed'] for r in results) else 1)
