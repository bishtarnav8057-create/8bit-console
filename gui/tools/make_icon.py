"""Builds assets/icon/pixel8.ico from the Logo grid in src/art/Sprites.cpp (the first sprite), so the
exe icon is the same pixel art as the app. Run once after changing the logo:  python tools/make_icon.py
Needs Pillow (pip install pillow)."""
import os
import re

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(HERE, '..', 'src', 'art', 'Sprites.cpp'), encoding='utf-8').read()

# palette: case 'o': return 0x174D2B;
palette = {c: int(v, 16) for c, v in re.findall(r"case '(.)': return 0x([0-9A-Fa-f]{6});", src)}
# the Logo grid: the first 16 quoted rows of 16 chars
rows = re.findall(r'"([^"]{16})"', src)[:16]

logo = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
for y, row in enumerate(rows):
    for x, ch in enumerate(row):
        if ch in palette:
            v = palette[ch]
            logo.putpixel((x, y), ((v >> 16) & 255, (v >> 8) & 255, v & 255, 255))

sizes = [16, 32, 48, 64, 128, 256]   # whole multiples of 16, nearest-neighbour, so it stays sharp
images = [logo.resize((s, s), Image.NEAREST) for s in sizes]
out = os.path.join(HERE, '..', 'assets', 'icon', 'pixel8.ico')
images[-1].save(out, sizes=[(s, s) for s in sizes], append_images=images[:-1])
print('wrote', os.path.normpath(out))
