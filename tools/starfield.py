#!/usr/bin/env python3
"""Draw the transit's two starfield layers (2048 x 2048) and convert them for the game:

  tools/starfield.py    writes art/stars_far.png, art/stars_near.png and
                        DayZPortals/data/stars_far_co.paa, stars_near_ca.paa

stars_far: deep black, thousands of faint stars, a Milky Way band, a breath of nebula.
stars_near: transparent, a few hundred bright stars with glow - zoomed faster than the far
layer in transit, so they rush past. The .paa conversion is tools/danger_sign.py's.
"""
import math, os, random, sys
from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from danger_sign import ROOT, to_paa

N = 2048


def star_colour(rnd, level):
    tint = rnd.choice([(1.0, 1.0, 1.0)] * 5 + [(0.8, 0.88, 1.0), (1.0, 0.9, 0.75), (1.0, 0.8, 0.7)])
    return tuple(int(min(255, level * c)) for c in tint)


def far():
    rnd = random.Random(47)
    img = Image.new('RGB', (N, N), (2, 2, 6))
    # a Milky Way band across the diagonal: soft lumpy light with dark dust lanes
    band = Image.new('L', (N, N), 0)
    bd = ImageDraw.Draw(band)
    for _ in range(900):
        t = rnd.random()
        x, y = t * N, (1 - t) * N * 0.9 + N * 0.05
        off = rnd.gauss(0, N * 0.07)
        x, y = x + off * 0.7, y + off * 0.7
        r = rnd.uniform(20, 90)
        bd.ellipse([x - r, y - r, x + r, y + r], fill=rnd.randint(20, 55))
    band = band.filter(ImageFilter.GaussianBlur(40))
    img = Image.composite(Image.new('RGB', (N, N), (150, 142, 170)), img, band)
    dust = Image.new('L', (N, N), 0)
    dd = ImageDraw.Draw(dust)
    for _ in range(140):
        t = rnd.random()
        x, y = t * N + rnd.gauss(0, 30), (1 - t) * N * 0.9 + N * 0.05 + rnd.gauss(0, 30)
        r = rnd.uniform(15, 50)
        dd.ellipse([x - r, y - r * 0.5, x + r, y + r * 0.5], fill=rnd.randint(60, 140))
    img = Image.composite(Image.new('RGB', (N, N), (2, 2, 6)), img, dust.filter(ImageFilter.GaussianBlur(18)))
    # nebula wisps, red and blue
    for colour, seed in (((120, 30, 70), 1), ((30, 60, 130), 2)):
        neb = Image.new('L', (N, N), 0)
        nd = ImageDraw.Draw(neb)
        r2 = random.Random(seed)
        cx, cy = r2.uniform(0.2, 0.8) * N, r2.uniform(0.2, 0.8) * N
        for _ in range(160):
            x, y, r = cx + r2.gauss(0, N * 0.12), cy + r2.gauss(0, N * 0.08), r2.uniform(20, 120)
            nd.ellipse([x - r, y - r, x + r, y + r], fill=r2.randint(10, 32))
        img = Image.composite(Image.new('RGB', (N, N), colour), img, neb.filter(ImageFilter.GaussianBlur(60)))
    # stars: thousands of faint points, denser in the band
    d = ImageDraw.Draw(img)
    for _ in range(16000):
        x, y = rnd.randrange(N), rnd.randrange(N)
        if band.getpixel((x, y)) < 6 and rnd.random() < 0.4:
            continue
        level = rnd.choice([90, 110, 140, 170, 200, 240]) * rnd.uniform(0.75, 1.0)
        c = star_colour(rnd, level)
        if level > 150 and rnd.random() < 0.3:
            d.ellipse([x - 1, y - 1, x + 1, y + 1], fill=c)
        else:
            d.point((x, y), fill=c)
    return img


def near():
    rnd = random.Random(48)
    img = Image.new('RGBA', (N, N), (0, 0, 0, 0))
    glow = Image.new('RGBA', (N, N), (0, 0, 0, 0))
    d, g = ImageDraw.Draw(img), ImageDraw.Draw(glow)
    for _ in range(450):
        x, y = rnd.randrange(N), rnd.randrange(N)
        size = rnd.choice([1, 1, 1.5, 2, 2.5, 3.5])
        c = star_colour(rnd, 255)
        d.ellipse([x - size, y - size, x + size, y + size], fill=c + (255,))
        if size >= 2:
            r = size * 5
            g.ellipse([x - r, y - r, x + r, y + r], fill=c + (70,))
    glow = glow.filter(ImageFilter.GaussianBlur(6))
    return Image.alpha_composite(glow, img)


if __name__ == '__main__':
    art = os.path.join(ROOT, 'art')
    data = os.path.join(ROOT, 'DayZPortals/data')
    os.makedirs(art, exist_ok=True)
    far().save(os.path.join(art, 'stars_far.png'))
    near().save(os.path.join(art, 'stars_near.png'))
    print('wrote art/stars_far.png, art/stars_near.png')
    if '--no-paa' not in sys.argv:
        to_paa(os.path.join(art, 'stars_far.png'), os.path.join(data, 'stars_far_co.paa'))
        to_paa(os.path.join(art, 'stars_near.png'), os.path.join(data, 'stars_near_ca.paa'))
        print('wrote DayZPortals/data/stars_far_co.paa, stars_near_ca.paa')
