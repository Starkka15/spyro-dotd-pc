"""Brightness and frame-to-frame change for a screenshot burst, to spot flicker frames.

usage: python -I frame_stats.py <glob>   e.g. H:/spyro-dotd/logs/intro-*.png
A frame much darker/brighter than both neighbours, or a big jump that reverts, is flagged.
"""
import glob, sys
from PIL import Image, ImageChops, ImageStat

files = sorted(glob.glob(sys.argv[1]))
imgs = [Image.open(f).convert('L').resize((320, 180)) for f in files]
mean = [ImageStat.Stat(i).mean[0] for i in imgs]
diff = [0.0] + [ImageStat.Stat(ImageChops.difference(imgs[k], imgs[k - 1])).mean[0] for k in range(1, len(imgs))]
for k, f in enumerate(files):
    flag = ''
    if 0 < k < len(files) - 1:
        a, b, c = mean[k - 1], mean[k], mean[k + 1]
        if (b < a - 8 and b < c - 8) or (b > a + 8 and b > c + 8):
            flag = '  <-- spike'
        elif diff[k] > 20 and diff[k + 1] > 20:
            flag = '  <-- changes and reverts'
    print(f'{f.split("-")[-1][:-4]:>6}  mean {mean[k]:6.1f}  diff {diff[k]:5.1f}{flag}')
