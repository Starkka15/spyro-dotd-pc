"""Shows dumped 8-bit video planes (texdump_NN_BASE_WxH_pPITCH.raw) as one contact sheet.

usage: python -I raw_planes.py <dir> <out.png> [first_index] [count]
Each plane is drawn with the pitch from its name; the rightmost (pitch - width) bytes of each
row are shown too, separated by a red line, to see whether data spills into the padding.
"""
import glob, os, re, sys
from PIL import Image, ImageDraw

d, out = sys.argv[1], sys.argv[2]
first = int(sys.argv[3]) if len(sys.argv) > 3 else 0
count = int(sys.argv[4]) if len(sys.argv) > 4 else 6
files = sorted(glob.glob(os.path.join(d, 'texdump_*.raw')))[first:first + count]
tiles = []
for f in files:
    m = re.search(r'texdump_(\d+)_([0-9A-F]{8})_(\d+)x(\d+)_p(\d+)\.raw', f)
    idx, base, w, h, pitch = m.group(1), m.group(2), int(m.group(3)), int(m.group(4)), int(m.group(5))
    data = open(f, 'rb').read()
    rows = min(h, len(data) // pitch)
    im = Image.frombytes('L', (pitch, rows), data[:pitch * rows])
    im = im.convert('RGB')
    ImageDraw.Draw(im).line([(w, 0), (w, rows)], fill=(255, 0, 0))
    im = im.resize((pitch * 360 // rows, 360) if rows else (10, 10))
    ImageDraw.Draw(im).text((4, 4), f'{idx} {base} {w}x{h} p{pitch}', fill=(0, 255, 0))
    tiles.append(im)
W = sum(t.width for t in tiles[:3])
sheet = Image.new('RGB', (max(W, 1), 360 * ((len(tiles) + 2) // 3)))
x = y = 0
for i, t in enumerate(tiles):
    if i and i % 3 == 0:
        x, y = 0, y + 360
    sheet.paste(t, (x, y))
    x += t.width
sheet.save(out)
print('saved', out)
