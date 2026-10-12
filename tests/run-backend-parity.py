#!/usr/bin/env python3
"""Compare AST-to-C and IR-native runtime outputs for the original examples."""
import pathlib, subprocess, sys, tempfile, json
root=pathlib.Path(__file__).resolve().parents[1]
compiler=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else root.parent/'build'/'shalimarc'
rows=[]
with tempfile.TemporaryDirectory() as d:
    for source in sorted((root/'tests/spec-examples').glob('*.shm')):
        item={'program':source.name}
        for backend in ('ast','ir'):
            c=pathlib.Path(d)/(source.stem+'-'+backend+'.c')
            cmd=[str(compiler),str(source),'--emit-c',str(c),'--release']
            if backend=='ir': cmd.append('--ir-native')
            p=subprocess.run(cmd,capture_output=True,text=True,timeout=20)
            item[backend+'_emit']=p.returncode
            if p.returncode:
                item[backend+'_diagnostic']=(p.stderr or p.stdout).strip()[:500]
                continue
            exe=c.with_suffix('')
            p=subprocess.run(['cc','-std=c99',str(c),'-lm','-o',str(exe)],capture_output=True,text=True,timeout=20)
            item[backend+'_compile']=p.returncode
            if p.returncode:
                item[backend+'_diagnostic']=p.stderr[:500]
                continue
            try:
                p=subprocess.run([str(exe)],capture_output=True,text=True,timeout=5)
                item[backend+'_run']=p.returncode
                item[backend+'_output']=p.stdout
            except subprocess.TimeoutExpired:
                item[backend+'_run']='timeout'
        item['parity']=item.get('ast_run')==0 and item.get('ir_run')==0 and item.get('ast_output')==item.get('ir_output')
        rows.append(item)
print(json.dumps([{k:v for k,v in r.items() if not k.endswith('_output')} for r in rows],indent=2))
print('Exact parity: %d/%d' %(sum(r['parity'] for r in rows),len(rows)))
sys.exit(0 if all(r['parity'] for r in rows) else 2)
