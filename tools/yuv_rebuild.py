"""Rebuilds RGB from dumped Bink planes: Y (1280x720 p1280) + two 640x360 p768 planes.

usage: python -I yuv_rebuild.py <dir> <out.png> <y_idx> <p1_idx> <p2_idx>
Writes two variants side by side: p1 as Cr / p2 as Cb, and swapped (BT.601 limited range).
"""
import glob, os, sys
import numpy as np
from PIL import Image

d, out = sys.argv[1], sys.argv[2]
idx = [int(a) for a in sys.argv[3:6]]
def load(i, w, h, pitch):
    f = glob.glob(os.path.join(d, f'texdump_{i:02d}_*.raw'))[0]
    raw = open(f, 'rb').read()
    raw = raw + bytes(max(0, pitch * h - len(raw)))  # last row is not padded to the pitch
    a = np.frombuffer(raw, np.uint8)[:pitch * h].reshape(h, pitch)[:, :w]
    return a.astype(np.float32), os.path.basename(f)
y, ny = load(idx[0], 1280, 720, 1280)
p1, n1 = load(idx[1], 640, 360, 768)
p2, n2 = load(idx[2], 640, 360, 768)
print(ny, n1, n2)
for n, p in ((n1, p1), (n2, p2)):
    print(n, 'mean %.1f min %d max %d' % (p.mean(), p.min(), p.max()))
up = lambda p: p.repeat(2, 0).repeat(2, 1)
def rgb(cr, cb):
    yy = (y - 16) * 1.164
    cr, cb = up(cr) - 128, up(cb) - 128
    r = yy + 1.596 * cr
    g = yy - 0.392 * cb - 0.813 * cr
    b = yy + 2.017 * cb
    return np.clip(np.stack([r, g, b], -1), 0, 255).astype(np.uint8)
a = rgb(p1, p2)
b = rgb(p2, p1)
Image.fromarray(np.concatenate([a, b], 1)).resize((1280, 360)).save(out)
print('saved', out)
