"""Counts colour flicker in a frame burst: frames whose red-blue tint jumps away from both
neighbours (the Bink-video bug: every other frame tinted or washed out).

usage: python -I tint_flicker.py <glob> [-v]
"""
import glob, sys
from PIL import Image, ImageStat

files = sorted(glob.glob(sys.argv[1]))
tint, lum = [], []
for f in files:
    im = Image.open(f).convert('RGB').resize((160, 90))
    r, g, b = ImageStat.Stat(im).mean
    tint.append(r - b)
    lum.append((r + g + b) / 3)
bad = 0
for k in range(1, len(files) - 1):
    jt = (tint[k] - tint[k - 1]) * (tint[k] - tint[k + 1])
    jl = (lum[k] - lum[k - 1]) * (lum[k] - lum[k + 1])
    odd = (jt > 15 * 15 and min(abs(tint[k] - tint[k - 1]), abs(tint[k] - tint[k + 1])) > 15) or \
          (jl > 25 * 25 and min(abs(lum[k] - lum[k - 1]), abs(lum[k] - lum[k + 1])) > 25)
    bad += odd
    if '-v' in sys.argv:
        print(f'{files[k][-14:]} tint {tint[k]:6.1f} lum {lum[k]:6.1f}{"  <-- odd" if odd else ""}')
print(f'{bad} odd frames of {len(files)}')
