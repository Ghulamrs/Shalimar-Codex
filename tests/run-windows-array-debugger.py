"""Execute source-breakpoint and typed local inspection checks under Windows CDB."""
import argparse
import ctypes
import json
from pathlib import Path
import re
import subprocess
import sys

if len(sys.argv)>1 and sys.argv[1]=='--child':
    dll=ctypes.CDLL(sys.argv[2])
    dll.compute.argtypes=[ctypes.c_int]; dll.compute.restype=ctypes.c_int
    print('SHM_RESULT='+str(dll.compute(int(sys.argv[3]))),flush=True)
    raise SystemExit(0)
parser=argparse.ArgumentParser()
parser.add_argument('--compiler-build',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--configuration',choices=['Debug','Release'])
args=parser.parse_args()
build=args.compiler_build.resolve(); output=args.output.resolve(); output.mkdir(parents=True,exist_ok=True)
link=Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/link.exe')
cdb=Path('C:/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe')
kernel=Path('C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/kernel32.lib')
def run(command):
    result=subprocess.run(list(map(str,command)),capture_output=True,text=True,timeout=45)
    if result.returncode: raise RuntimeError(result.stdout+result.stderr)
    return result.stdout+result.stderr
def verify_unwind(dll):
    library=ctypes.CDLL(str(dll))
    fn=ctypes.cast(library.compute,ctypes.c_void_p).value
    nt=ctypes.WinDLL('ntdll')
    lookup=nt.RtlLookupFunctionEntry
    lookup.argtypes=[ctypes.c_uint64,ctypes.POINTER(ctypes.c_uint64),ctypes.c_void_p]
    lookup.restype=ctypes.c_void_p
    unwind=nt.RtlVirtualUnwind
    unwind.argtypes=[ctypes.c_uint32,ctypes.c_uint64,ctypes.c_uint64,ctypes.c_void_p,ctypes.c_void_p,ctypes.POINTER(ctypes.c_void_p),ctypes.POINTER(ctypes.c_uint64),ctypes.c_void_p]
    unwind.restype=ctypes.c_void_p
    base=ctypes.c_uint64(); entry=lookup(fn,ctypes.byref(base),None)
    assert entry, 'Missing Windows runtime function entry'
    frame=ctypes.c_uint32.from_address(fn+7).value
    stack=ctypes.create_string_buffer(frame+512); fp=ctypes.addressof(stack)+frame+256
    saved=0xabcdef; returned=0x12345678
    ctypes.c_uint64.from_address(fp).value=saved
    ctypes.c_uint64.from_address(fp+8).value=returned
    # Partial prologues, ordinary body, and extra temporary stack allocation.
    scenarios=[(0,fp+8,saved),(1,fp,saved),(4,fp,fp),(11,fp-frame,fp),(11,fp-frame-32,fp)]
    for offset,rsp,rbp in scenarios:
        storage=ctypes.create_string_buffer(1250); ctx=(ctypes.addressof(storage)+15)&~15
        for field,value in [(152,rsp),(160,rbp),(248,fn+offset)]:
            ctypes.c_uint64.from_address(ctx+field).value=value
        data=ctypes.c_void_p(); establisher=ctypes.c_uint64()
        unwind(0,base.value,fn+offset,entry,ctx,ctypes.byref(data),ctypes.byref(establisher),None)
        observed=[ctypes.c_uint64.from_address(ctx+k).value for k in [152,160,248]]
        assert observed==[fp+16,saved,returned], (offset,observed)
    return len(scenarios)
results=[]
for config in ([args.configuration] if args.configuration else ['Debug','Release']):
 for mode in ['debug','release']:
  folder=output/(config+'-'+mode);folder.mkdir(exist_ok=True)
  source=folder/'array.shl'
  source.write_text('fun <int> = compute(i: int) {\nint A[3] : {10,20,30}\nint B[2] : {7,9}\nreal R[3] : {1.25,2.5,3.75}\nreal T[2] : {0.1,0.2}\nint x : A[i]\nreturn x+B[1]\n}\nfun main() {}\n')
  obj=folder/'debugfixture.obj';dll=folder/'debugfixture.dll'
  run([build/config/'shalimarc.exe',source,'--object',obj,'--'+mode,'--debug-symbols'])
  run([link,'/nologo','/dll','/noentry','/nodefaultlib','/debug','/incremental:no','/export:compute','/out:'+str(dll),obj,build/config/'shalimar_runtime.lib',kernel])
  unwind_checks=verify_unwind(dll)
  large_source=folder/'large.shl'; large_obj=folder/'large.obj'; large_dll=folder/'large.dll'
  large_source.write_text('fun <int> = compute(i: int) { int A[100] return A[i] }\nfun main() {}\n')
  run([build/config/'shalimarc.exe',large_source,'--object',large_obj,'--'+mode,'--debug-symbols'])
  run([link,'/nologo','/dll','/noentry','/nodefaultlib','/debug','/incremental:no','/export:compute','/out:'+str(large_dll),large_obj,build/config/'shalimar_runtime.lib',kernel])
  unwind_checks+=verify_unwind(large_dll)
  small_source=folder/'small.shl'; small_obj=folder/'small.obj'; small_dll=folder/'small.dll'
  small_source.write_text('fun <int> = compute(i: int) { int A[3] return A[i] }\nfun main() {}\n')
  run([build/config/'shalimarc.exe',small_source,'--object',small_obj,'--'+mode,'--debug-symbols'])
  run([link,'/nologo','/dll','/noentry','/nodefaultlib','/debug','/incremental:no','/export:compute','/out:'+str(small_dll),small_obj,build/config/'shalimar_runtime.lib',kernel])
  unwind_checks+=verify_unwind(small_dll)
  for index in [0,2]:
   command=f'.lines -e; sxe ld:debugfixture; g; .reload /f debugfixture.dll; bp `{source}:7`; g; bc *; dv /t /v; ?? A[0]; ?? A[1]; ?? A[2]; ?? B[0]; ?? B[1]; ?? R[0]; ?? R[1]; ?? R[2]; ?? T[0]; ?? T[1]; ?? i; ?? x; k; gu; .echo SHM_STEPOUT; g; q'
   log=run([cdb,'-y',folder,'-c',command,sys.executable,Path(__file__).resolve(),'--child',dll,index])
   (folder/('cdb-'+str(index)+'.log')).write_text(log)
   values=[int(v) for v in re.findall(r'^int 0n(-?\d+)\s*$',log,re.M)]
   real_values=[float(v) for v in re.findall(r'^double ([+-]?[0-9.eE]+)\s*$',log,re.M)]
   real_ok=(real_values==[1.25,2.5,3.75,0.1,0.2] and 'double [3] R' in log and 'double [2] T' in log)
   expected=[10,20,30,7,9,index,[10,20,30][index]]
   passed=(real_ok and values==expected and 'int [3] A' in log and 'int [2] B' in log and 'SHM_RESULT='+str(expected[-1]+9) in log and 'Breakpoint 0 hit' in log and re.search(r'^SHM_STEPOUT\s*$',log,re.M) is not None)
   results.append(dict(configuration=config,mode=mode,index=index,values=values,real_values=real_values,unwind_checks=unwind_checks,passed=passed))
(output/'results.json').write_text(json.dumps(results,indent=2))
print(str(sum(r['passed'] for r in results))+'/'+str(len(results))+' Windows array debugger scenarios passed')
raise SystemExit(0 if all(r['passed'] for r in results) else 1)
