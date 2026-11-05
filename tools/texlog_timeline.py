"""Per-texture event timeline from a --texture_log_loads log.

usage: python -I texlog_timeline.py <game.log> [base_hex ...]
For each video texture (k_8 1280x720 / 640x360), counts CPU-write faults that touch it,
uploads of its pages, outdated events and loads, then prints the event sequence of the
given bases (first 120 events each). F = fault, U = upload, O = outdated, L = load.
"""
import re, sys, os; START=int(os.environ.get("TL_START","0"))
from collections import defaultdict

lines = open(sys.argv[1], encoding='utf-8', errors='replace').read().splitlines()
tex = {}  # base -> (end, label)
ev = []
for ln in lines:
    m = re.search(r'texlog (load|outdated) (\S+) (\d+)x(\d+) (?:\w+ )?base ([0-9A-F]{8})(?: size ([0-9A-F]+))?', ln)
    if m:
        kind, fmt, w, h, base, size = m.groups()
        base = int(base, 16)
        if fmt == 'k_8' and (w, h) in (('1280', '720'), ('640', '360')):
            if size:
                tex[base] = (base + int(size, 16) - 1, f'{w}x{h}')
            ev.append(('L' if kind == 'load' else 'O', base, base, ln[12:23]))
        continue
    m = re.search(r'texlog fault ([0-9A-F]{8})\+([0-9A-F]+) exact=\d -> invalidate ([0-9A-F]{8})-([0-9A-F]{8})', ln)
    if m:
        ev.append(('F', int(m.group(3), 16), int(m.group(4), 16), ln[12:23], int(m.group(1), 16)))
        continue
    m = re.search(r'texlog upload ([0-9A-F]{8})-([0-9A-F]{8})', ln)
    if m:
        ev.append(('U', int(m.group(1), 16), int(m.group(2), 16), ln[12:23]))

counts = defaultdict(lambda: defaultdict(int))
seq = defaultdict(list)
for e in ev:
    k, a, b, t = e[0], e[1], e[2], e[3]
    for base, (end, label) in tex.items():
        if k in 'LO':
            hit = a == base
        else:
            hit = a <= end and b >= base
        if hit:
            counts[base][k] += 1
            note = f'{k}' if k in 'LO' else f'{k}[{a:08X}-{b:08X}' + (f' @{e[4]:08X}]' if k == 'F' else ']')
            seq[base].append(f'{t} {note}')
for base in sorted(tex):
    c = counts[base]
    print(f'{base:08X} {tex[base][1]:9}  faults {c["F"]:4}  uploads {c["U"]:4}  outdated {c["O"]:4}  loads {c["L"]:4}')
for arg in sys.argv[2:]:
    base = int(arg, 16)
    print(f'--- {base:08X}')
    for s in seq[base][START:START+120]:
        print(' ', s)
