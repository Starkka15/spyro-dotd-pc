"""Find functions the ReXGlue analyzer missed: code addresses stored in the image's data
(function pointers, callback tables, non-RTTI vtables) that are not registered functions.

usage: python -I find_funcs.py <image.bin> <generated/default/<name>_init.cpp> [--toml out.toml]
image.bin is the decompressed image from SPYRO_DUMP_IMAGE (loaded at 0x82000000).
"""
import re, struct, sys
import capstone

BASE = 0x82000000
img = open(sys.argv[1], 'rb').read()
init = open(sys.argv[2], encoding='utf-8').read()
out_toml = sys.argv[sys.argv.index('--toml') + 1] if '--toml' in sys.argv else None
known = {int(m, 16) for m in re.findall(r'\{ 0x([0-9A-Fa-f]{8}), ', init)}

# PE headers at the image start.
pe = struct.unpack_from('<I', img, 0x3C)[0]
assert img[pe:pe + 4] == b'PE\0\0', 'no PE header in the dump'
nsec = struct.unpack_from('<H', img, pe + 6)[0]
opt = struct.unpack_from('<H', img, pe + 20)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + opt + i * 40
    name = img[o:o + 8].rstrip(b'\0').decode('latin1')
    vsize, va, rawsize, rawptr = struct.unpack_from('<IIII', img, o + 8)
    ch = struct.unpack_from('<I', img, o + 36)[0]
    secs.append((name, BASE + va, vsize, bool(ch & 0x20000000)))
code = [(s, a, n) for s, a, n, x in secs if x]
# .reloc holds relocation entries and .pdata the exception table, not pointers the game calls.
data = [(s, a, n) for s, a, n, x in secs if not x and s not in ('.reloc', '.pdata')]
for s, a, n, x in secs:
    print(f'{s:8} {a:08X}-{a + n:08X} {"code" if x else "data"}')


def in_code(v):
    return any(a <= v < a + n for _, a, n in code)


def word(addr):
    return struct.unpack_from('>I', img, addr - BASE)[0]


def ends_block(w):
    # blr, bctr, b (unconditional, no link), or padding/zero before the address
    return w in (0x4E800020, 0x4E800420, 0) or (w >> 26 == 18 and not (w & 1))


md = capstone.Cs(capstone.CS_ARCH_PPC, capstone.CS_MODE_32 | capstone.CS_MODE_BIG_ENDIAN)
refs = {}
for s, a, n in data:
    for off in range(0, n - 3, 4):
        v = word(a + off)
        if v & 3 or not in_code(v) or v in known:
            continue
        refs.setdefault(v, []).append(a + off)

# Addresses built in code: lis rX,hi then addi/ori rY,rX,lo (callbacks passed in registers).
# A linear sweep keeps the last lis per register; any write to that register drops it.
hi = {}
for s, a, n in code:
    for off in range(0, n - 3, 4):
        pc = a + off
        w = word(pc)
        op, rd, ra, imm = w >> 26, (w >> 21) & 31, (w >> 16) & 31, w & 0xFFFF
        if op == 15 and ra == 0:  # lis rd, imm
            hi[rd] = (imm << 16) & 0xFFFFFFFF
            continue
        v = None
        if op == 14 and ra in hi:  # addi rd, ra, simm
            v = (hi[ra] + (imm - 0x10000 if imm & 0x8000 else imm)) & 0xFFFFFFFF
        elif op == 24 and rd in hi:  # ori ra, rs, uimm (rs is the rd field)
            v = hi[rd] | imm
        if v is not None and not v & 3 and in_code(v) and v not in known:
            refs.setdefault(v, []).append(pc)
        if op in (14, 15) or w >> 26 in (32, 33, 34, 35, 40, 41, 42, 43, 31):
            hi.pop(rd, None)  # rd overwritten by a load/arith
        if op == 18 or op == 16 or w in (0x4E800020, 0x4E800420):
            hi.clear()  # branch: the register state is unknown on the other side

import bisect
starts = sorted(known)


def owner(addr):
    i = bisect.bisect_right(starts, addr) - 1
    return starts[i] if i >= 0 else None


# A code reference from the same function as its target is a computed jump to a label
# (lis/addi; mtctr; bctr), not a function pointer. Keep code refs only from other functions,
# and data refs always.
def scope_entry(r, v):
    # SEH scope tables hold a function's try-range begin/end next to its handler address,
    # so a neighbouring word inside the same function marks an exception funclet.
    for d in (-12, -8, -4, 4, 8, 12):
        try:
            n = word(r + d)
        except struct.error:
            continue
        # Try-range bounds sit mid-code; a neighbour that is itself a function start
        # (after blr/b/bctr) is another table slot, e.g. a vtable of unregistered functions.
        # A neighbour that is a registered function start is a table slot too (seen in a C++
        # static-initializer table: a thunk right after a registered entry, merged into it).
        if (n != v and in_code(n) and n not in known and owner(n) == owner(v)
                and not ends_block(word(n - 4))):
            return True
    return False


for v in list(refs):
    keep = [r for r in refs[v] if (not in_code(r) or owner(r) != owner(v))
            and (in_code(r) or not scope_entry(r, v))]
    if keep:
        refs[v] = keep
    else:
        del refs[v]

found, skipped = [], []
for v in sorted(refs):
    prev = word(v - 4)
    insns = list(md.disasm(img[v - BASE:v - BASE + 16], v))
    # A jump table after a bctr holds code addresses (0x82xxxxxx decodes as lwz); not a function.
    table = in_code(word(v)) or in_code(word(v + 4))
    # Short thunks (lis/addi/b, then zero padding) decode fewer than 4 instructions; fine as
    # long as the decoded run ends in a branch.
    whole = len(insns) == 4 or (insns and insns[-1].mnemonic in ('b', 'blr', 'bctr'))
    ok = ends_block(prev) and whole and not table
    (found if ok else skipped).append(v)

print(f'{len(known)} registered, {len(refs)} unregistered code addresses in data, '
      f'{len(found)} look like function starts, {len(skipped)} rejected')
for v in found:
    i = next(md.disasm(img[v - BASE:v - BASE + 4], v))
    print(f'  {v:08X}  {i.mnemonic} {i.op_str}   refs {len(refs[v])} e.g. {refs[v][0]:08X}')
for v in skipped[:15]:
    print(f'  rejected {v:08X} prev {word(v - 4):08X}')
if out_toml:
    with open(out_toml, 'w', newline='\n') as f:
        f.write('# Generated by tools/find_funcs.py: code addresses referenced from data\n')
        f.write('# that the analyzer did not register (function pointers, callback tables).\n')
        f.write('[functions]\n')
        for v in found:
            f.write(f'0x{v:08X} = {{}}\n')
    print('wrote', out_toml)
