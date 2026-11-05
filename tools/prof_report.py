"""Symbolizes a sampler.ps1 profile and prints self time per function, plus game-code callers.

usage: python -I prof_report.py <prof.tsv> <exe_dir> [top]
Leaf samples are grouped by function (llvm-symbolizer on each module's PDB); "blamed" adds DLL/ntdll
samples to the game function found first on the stack.
"""
import os, re, subprocess, sys
from collections import Counter

SYM = r'G:\LLVM\clang+llvm-20.1.8-x86_64-pc-windows-msvc\bin\llvm-symbolizer.exe'
prof, exe_dir = sys.argv[1], sys.argv[2]
top = int(sys.argv[3]) if len(sys.argv) > 3 else 30
lines = open(prof, encoding='utf-8-sig').read().splitlines()
rows = []
for ln in lines:
    parts = ln.split('\t')
    if len(parts) == 3 and parts[0].isdigit():
        rows.append((int(parts[0]), parts[1], parts[2]))
total = sum(r[0] for r in rows)

addrs = {}
for _, leaf, caller in rows:
    for a in (leaf, caller):
        m = re.match(r'(.+)\+([0-9A-F]+)$', a)
        if m:
            addrs.setdefault(m.group(1), set()).add(m.group(2))
names = {}
for mod, offs in addrs.items():
    path = os.path.join(exe_dir, mod)
    if not os.path.exists(path):
        for o in offs:
            names[f'{mod}+{o}'] = mod
        continue
    offs = sorted(offs)
    out = subprocess.run([SYM, f'--obj={path}', '--relative-address', '--functions=short', '--no-inlines'],
                         input='\n'.join('0x' + o for o in offs), capture_output=True, text=True).stdout
    blocks = [b for b in out.strip().split('\n\n')]
    for o, b in zip(offs, blocks):
        fn = b.splitlines()[0] if b else '??'
        names[f'{mod}+{o}'] = f'{mod}!{fn}' if fn != '??' else f'{mod}+{o}'

self_t, blamed = Counter(), Counter()
for n, leaf, caller in rows:
    lf = names.get(leaf, leaf)
    self_t[lf] += n
    if lf.startswith('spyro_dotd'):
        blamed[lf] += n
    else:
        blamed[names.get(caller, caller) + '  (via ' + lf.split('!')[0].split('+')[0] + ')'] += n
print(f'{total} samples\n--- self time')
for f, n in self_t.most_common(top):
    print(f'{100*n/total:5.1f}%  {f}')
print('--- by game function (DLL time charged to first game frame on stack)')
for f, n in blamed.most_common(top):
    print(f'{100*n/total:5.1f}%  {f}')
