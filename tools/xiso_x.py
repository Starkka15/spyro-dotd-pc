"""Extract the game partition of an Xbox 360 disc image (XDVDFS) to a folder.

usage: python -I xiso_x.py <disc.iso> <out_dir>
Finds the partition itself: plain XISO (0), XGD3 (0x2080000), XGD2 (0xFD90000), XGD1 (0x18300000).
"""
import os, struct, sys

iso, out = sys.argv[1:3]
f = open(iso, 'rb')
C = 1 << 22


def rd(o, n):
    f.seek(o)
    return f.read(n)


for B in (0, 0x2080000, 0xFD90000, 0x18300000):
    v = rd(B + 32 * 2048, 2048)
    if v[:20] == b'MICROSOFT*XBOX*MEDIA':
        break
else:
    sys.exit('no XDVDFS volume descriptor found')
print('partition at', hex(B))
root, rsz = struct.unpack('<II', v[20:28])
nf = nb = nd = 0


def walk(sec, size, dir):
    global nf, nb, nd
    d = rd(B + sec * 2048, size)
    seen = set()
    st = [0]
    while st:
        off = st.pop()
        if off in seen or off + 14 > len(d):
            continue
        seen.add(off)
        l, r, s, z, a, n = struct.unpack('<HHIIBB', d[off:off + 14])
        if l == 0xFFFF:
            continue
        name = d[off + 14:off + 14 + n].decode('latin1')
        if not name or name in ('.', '..') or any(c in name for c in '/\\:'):
            sys.exit('bad name %r' % name)
        if l:
            st.append(l * 4)
        if r:
            st.append(r * 4)
        p = os.path.join(dir, name)
        if a & 0x10:
            os.makedirs(p, exist_ok=True)
            nd += 1
            if z:
                walk(s, z, p)
        else:
            with open(p, 'xb') as o:
                f.seek(B + s * 2048)
                left = z
                while left:
                    b = f.read(min(C, left))
                    if not b:
                        sys.exit('short read ' + p)
                    o.write(b)
                    left -= len(b)
            nf += 1
            nb += z


os.makedirs(out, exist_ok=True)
walk(root, rsz, out)
got = sum(os.path.getsize(os.path.join(r, x)) for r, _, fs in os.walk(out) for x in fs)
cnt = sum(len(fs) for r, _, fs in os.walk(out))
print('files', nf, 'dirs', nd, 'bytes', nb, 'on disk', cnt, got, 'OK' if (cnt, got) == (nf, nb) else 'MISMATCH')
print(sorted(os.listdir(out)))
