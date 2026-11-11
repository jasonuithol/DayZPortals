#!/usr/bin/env python3
"""Draw the installation's danger billboard (2048 x 1024, the vanilla billboard face) and
convert it for the game:

  tools/danger_sign.py      writes art/danger_sign_co.png and DayZPortals/data/danger_sign_co.paa

ОПАСНО ("DANGER") on a red band, a prohibition circle over a bundle of dynamite, a pistol and
a grenade, and ВЗРЫВЧАТКА ЗАПРЕЩЕНА ("explosives prohibited" - the Russian sign wording for
"no explosives"); faded, peeling, rusted through at the edges and rivet holes, scorched. The .paa conversion is DayZ Tools' ImageToPAA under Proton's Wine.
"""
import math, os, random, subprocess, sys
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PNG = os.path.join(ROOT, 'art/danger_sign_co.png')
PAA = os.path.join(ROOT, 'DayZPortals/data/danger_sign_co.paa')
FONT = '/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf'
STEAM = os.environ.get('STEAM', os.path.expanduser('~/.steam/debian-installation'))
IMAGE_TO_PAA = os.path.join(STEAM, 'steamapps/common/DayZ Tools/Bin/ImageToPAA/ImageToPAA.exe')

W, H = 2048, 1024
RED = (178, 28, 24)
BLACK = (22, 20, 18)
PAPER = (232, 226, 208)


def text_centred(d, text, cy, size, fill, max_width):
    font = ImageFont.truetype(FONT, size)
    while d.textlength(text, font=font) > max_width:
        size -= 4
        font = ImageFont.truetype(FONT, size)
    box = d.textbbox((0, 0), text, font=font)
    d.text(((W - (box[2] - box[0])) / 2 - box[0], cy - (box[3] + box[1]) / 2), text, font=font, fill=fill)


def dynamite(d, cx, cy, s):
    """Three sticks taped together, a fuse with a spark."""
    for i in (-1, 0, 1):
        x = cx + i * 0.30 * s
        d.rounded_rectangle([x - 0.14 * s, cy - 0.55 * s, x + 0.14 * s, cy + 0.55 * s], radius=0.07 * s, fill=BLACK)
    for y in (-0.30, 0.30):
        d.rectangle([cx - 0.50 * s, cy + (y - 0.06) * s, cx + 0.50 * s, cy + (y + 0.06) * s], fill=PAPER)
        d.rectangle([cx - 0.46 * s, cy + (y - 0.035) * s, cx + 0.46 * s, cy + (y + 0.035) * s], fill=BLACK)
    pts = [(cx + 0.02 * s * math.sin(t * 6) + t * 0.25 * s, cy - 0.55 * s - t * 0.45 * s) for t in [i / 20 for i in range(21)]]
    d.line(pts, fill=BLACK, width=int(0.05 * s))
    sx, sy = pts[-1]
    for a in range(0, 360, 45):
        r = 0.14 * s if a % 90 == 0 else 0.09 * s
        d.line([sx, sy, sx + r * math.cos(math.radians(a)), sy + r * math.sin(math.radians(a))], fill=BLACK, width=int(0.035 * s))


def pistol(d, cx, cy, s):
    """A Makarov-ish pistol in profile, muzzle to the left."""
    slide = [cx - 0.75 * s, cy - 0.30 * s, cx + 0.55 * s, cy - 0.02 * s]
    d.rounded_rectangle(slide, radius=0.05 * s, fill=BLACK)
    grip = [(cx + 0.18 * s, cy - 0.04 * s), (cx + 0.55 * s, cy - 0.04 * s), (cx + 0.72 * s, cy + 0.62 * s), (cx + 0.36 * s, cy + 0.66 * s)]
    d.polygon(grip, fill=BLACK)
    d.arc([cx - 0.12 * s, cy - 0.16 * s, cx + 0.26 * s, cy + 0.30 * s], 0, 180, fill=BLACK, width=int(0.06 * s))
    d.rectangle([cx - 0.70 * s, cy - 0.36 * s, cx - 0.62 * s, cy - 0.28 * s], fill=BLACK)


def grenade(d, cx, cy, s):
    """An F1 'limonka': segmented body, fuse head, lever and ring."""
    body = [cx - 0.38 * s, cy - 0.40 * s, cx + 0.38 * s, cy + 0.60 * s]
    d.ellipse(body, fill=BLACK)
    for i in range(1, 4):
        y = body[1] + (body[3] - body[1]) * i / 4
        d.line([cx - 0.38 * s, y, cx + 0.38 * s, y], fill=PAPER, width=int(0.035 * s))
    for x in (-0.13, 0.13):
        d.line([cx + x * s, body[1] + 0.05 * s, cx + x * s, body[3] - 0.05 * s], fill=PAPER, width=int(0.035 * s))
    d.rectangle([cx - 0.14 * s, cy - 0.58 * s, cx + 0.14 * s, cy - 0.38 * s], fill=BLACK)
    d.polygon([(cx + 0.10 * s, cy - 0.56 * s), (cx + 0.20 * s, cy - 0.60 * s), (cx + 0.46 * s, cy + 0.10 * s), (cx + 0.38 * s, cy + 0.14 * s)], fill=BLACK)
    d.ellipse([cx - 0.42 * s, cy - 0.78 * s, cx - 0.12 * s, cy - 0.48 * s], outline=BLACK, width=int(0.045 * s))


def blotches(count, rmin, rmax, seed, near_edge=False):
    """An 'L' mask of soft irregular blotches (lumpy clusters of discs, then blurred)."""
    rnd = random.Random(seed)
    m = Image.new('L', (W, H), 0)
    d = ImageDraw.Draw(m)
    for _ in range(count):
        if near_edge:
            side = rnd.randrange(4)
            t = rnd.random()
            x, y = [(t * W, rnd.randint(0, 25)), (t * W, H - rnd.randint(0, 25)), (rnd.randint(0, 25), t * H), (W - rnd.randint(0, 25), t * H)][side]
        else:
            x, y = rnd.randrange(W), rnd.randrange(H)
        r = rnd.randint(rmin, rmax)
        for _ in range(rnd.randint(4, 9)):
            ox, oy, rr = rnd.gauss(0, r * 0.6), rnd.gauss(0, r * 0.6), r * rnd.uniform(0.3, 0.8)
            d.ellipse([x + ox - rr, y + oy - rr, x + ox + rr, y + oy + rr], fill=255)
    return m


def weather(img):
    """Forty years out there: faded chalky paint peeling in flakes, rust eating in from the edges
    and the rivet holes, grime, and a few scorch marks."""
    rnd = random.Random(47)
    rust_dark, rust, rust_light = (58, 32, 18), (118, 62, 30), (160, 96, 52)

    # the paint has faded chalky: half its colour gone, lightened, gone a little yellow
    img = ImageEnhance.Color(img).enhance(0.62)
    img = Image.blend(img, Image.new('RGB', img.size, (214, 204, 182)), 0.22)
    img = Image.composite(Image.new('RGB', img.size, (236, 230, 214)), img, blotches(60, 40, 140, 9).filter(ImageFilter.GaussianBlur(50)).point(lambda v: v * 0.35))

    # and grimy all over
    grime = blotches(160, 6, 26, 1).filter(ImageFilter.GaussianBlur(10)).point(lambda v: v * 0.25)
    img = Image.composite(Image.new('RGB', img.size, (90, 78, 60)), img, grime)

    # rust: eating in from the edges in ragged bites, deepest at the corners, plus paint chips
    edge_rust = blotches(300, 6, 22, 2, near_edge=True)
    corners = Image.new('L', img.size, 0)
    cd = ImageDraw.Draw(corners)
    for cx, cy in ((0, 0), (W, 0), (0, H), (W, H)):
        for _ in range(40):
            a, dist = rnd.uniform(0, math.pi * 2), abs(rnd.gauss(0, 70))
            rr = rnd.randint(8, 28)
            x, y = cx + math.cos(a) * dist, cy + math.sin(a) * dist
            cd.ellipse([x - rr, y - rr, x + rr, y + rr], fill=255)
    edge_rust = Image.composite(Image.new('L', img.size, 255), edge_rust, corners)
    chips = blotches(70, 3, 14, 4)
    rust_mask = Image.composite(Image.new('L', img.size, 255), edge_rust, chips).filter(ImageFilter.GaussianBlur(2))
    rust_tex = Image.new('RGB', img.size, rust)
    rust_tex = Image.composite(Image.new('RGB', img.size, rust_dark), rust_tex, blotches(300, 4, 18, 5).filter(ImageFilter.GaussianBlur(3)))
    rust_tex = Image.composite(Image.new('RGB', img.size, rust_light), rust_tex, blotches(200, 3, 10, 6).filter(ImageFilter.GaussianBlur(2)).point(lambda v: v * 0.6))
    img = Image.composite(rust_tex, img, rust_mask)

    # paint peeling off in flakes, mostly round the edges and the rust: mottled grey primer or
    # bare metal under them, a soft shadow below and a pale lifted lip along the top
    under_tex = Image.effect_noise(img.size, 40).convert('RGB')
    for k in range(40):
        rnd3 = random.Random(1000 + k)
        if k < 30:
            side = rnd3.randrange(4)
            t = rnd3.random()
            x, y = [(t * W, 20), (t * W, H - 20), (20, t * H), (W - 20, t * H)][side]
            x, y = x + rnd3.gauss(0, 45), y + rnd3.gauss(0, 45)
        else:
            x, y = rnd3.randrange(80, W - 80), rnd3.randrange(80, H - 80)
        r = rnd3.uniform(8, 34)
        n = rnd3.randint(14, 24)
        poly = []
        for v in range(n):
            a = 2 * math.pi * v / n + rnd3.uniform(-0.12, 0.12)
            rr = r * rnd3.uniform(0.3, 1.2)
            poly.append((x + math.cos(a) * rr * 1.4, y + math.sin(a) * rr * 0.75))
        mask = Image.new('L', img.size, 0)
        ImageDraw.Draw(mask).polygon(poly, fill=255)
        shadow = mask.filter(ImageFilter.GaussianBlur(3)).transform(img.size, Image.AFFINE, (1, 0, -3, 0, 1, -4)).point(lambda v: v * 0.55)
        img = Image.composite(Image.new('RGB', img.size, (60, 52, 44)), img, shadow)
        lip = mask.transform(img.size, Image.AFFINE, (1, 0, 2, 0, 1, 3)).point(lambda v: v * 0.8)
        img = Image.composite(Image.new('RGB', img.size, (240, 236, 224)), img, lip)
        base = rnd3.choice([(146, 146, 130), (132, 136, 124), (110, 104, 94)])
        under = Image.blend(Image.new('RGB', img.size, base), under_tex, 0.18)
        img = Image.composite(under, img, mask)
        d = ImageDraw.Draw(img)
        for _ in range(rnd3.randint(0, 3)):					# rust starting in it
            sx, sy, sr = x + rnd3.gauss(0, r * 0.3), y + rnd3.gauss(0, r * 0.25), rnd3.uniform(2, 6)
            d.ellipse([sx - sr, sy - sr, sx + sr, sy + sr], fill=rust)

    # rivet holes near the corners, each weeping rust down the board; more runs off the top
    runs = Image.new('L', img.size, 0)
    rd = ImageDraw.Draw(runs)
    holes = [(70, 70), (W - 70, 70), (70, H - 70), (W - 70, H - 70), (W / 2, 50)]
    for x, y in holes:
        for _ in range(3):
            x0 = x + rnd.randint(-10, 10)
            rd.line([x0, y, x0 + rnd.randint(-8, 8), y + rnd.randint(120, 380)], fill=rnd.randint(140, 220), width=rnd.randint(5, 14))
    for _ in range(36):
        x = rnd.randrange(W)
        rd.line([x, 0, x + rnd.randint(-10, 10), rnd.randint(60, 340)], fill=rnd.randint(90, 190), width=rnd.randint(3, 12))
    runs = runs.filter(ImageFilter.GaussianBlur(4))
    img = Image.composite(Image.new('RGB', img.size, rust), img, runs)
    d = ImageDraw.Draw(img)
    for x, y in holes:
        d.ellipse([x - 14, y - 14, x + 14, y + 14], fill=rust_dark)
        d.ellipse([x - 8, y - 8, x + 8, y + 8], fill=(20, 14, 10))

    # scorch marks: soot that fades out (streaked upwards, as smoke goes), a brown heat-tinted
    # ring, a darker blistered middle - never solid black
    for (x, y, r) in ((W * 0.17, H * 0.70, 130), (W * 0.84, H * 0.36, 105), (W * 0.63, H * 0.93, 80)):
        rnd2 = random.Random(int(x * 7 + y))
        soot = Image.new('L', img.size, 0)
        sd = ImageDraw.Draw(soot)
        for _ in range(26):
            ox, oy = rnd2.gauss(0, r * 0.4), rnd2.gauss(-r * 0.25, r * 0.45)
            rr = r * rnd2.uniform(0.15, 0.45)
            sd.ellipse([x + ox - rr, y + oy - rr * 1.3, x + ox + rr, y + oy + rr * 0.7], fill=rnd2.randint(120, 255))
        tint = soot.filter(ImageFilter.GaussianBlur(r * 0.45)).point(lambda v: v * 0.55)
        img = Image.composite(Image.new('RGB', img.size, (104, 66, 34)), img, tint)
        smoke = soot.filter(ImageFilter.GaussianBlur(r * 0.25)).point(lambda v: v * 0.6)
        img = Image.composite(Image.new('RGB', img.size, (34, 26, 20)), img, smoke)
        middle = soot.filter(ImageFilter.GaussianBlur(r * 0.08)).point(lambda v: max(0, v - 150) * 1.6)
        img = Image.composite(Image.new('RGB', img.size, (18, 14, 11)), img, middle)

    noise = Image.effect_noise(img.size, 22).convert('RGB')
    return Image.blend(img, noise, 0.07)


def draw():
    img = Image.new('RGB', (W, H), PAPER)
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, W, 230], fill=RED)
    text_centred(d, 'ОПАСНО', 118, 200, PAPER, W - 200)

    cx, cy, r = W / 2, 540, 280
    dynamite(d, cx - 140, cy - 10, 165)
    grenade(d, cx + 150, cy - 10, 155)
    pistol(d, cx - 20, cy + 135, 105)
    d.ellipse([cx - r, cy - r, cx + r, cy + r], outline=RED, width=58)
    a = math.radians(45)
    d.line([cx - r * math.cos(a), cy - r * math.sin(a), cx + r * math.cos(a), cy + r * math.sin(a)], fill=RED, width=58)

    text_centred(d, 'ВЗРЫВЧАТКА ЗАПРЕЩЕНА', 920, 150, BLACK, W - 160)
    return weather(img)


def to_paa(png, paa):
    """ImageToPAA.exe via Proton's Wine, in the prefix tools/sign_pbo.sh uses."""
    compat = os.path.join(STEAM, 'compatibilitytools.d')
    protons = sorted(p for p in os.listdir(compat) if 'proton' in p.lower())
    wine = os.path.join(compat, protons[-1], 'files/bin/wine64')
    env = dict(os.environ, WINEPREFIX=os.path.expanduser('~/.dayz-keys/wineprefix'), WINEDEBUG='-all', WINEDLLOVERRIDES='mscoree,mshtml=')
    winpath = lambda p: 'Z:' + os.path.abspath(p).replace('/', '\\')
    os.makedirs(os.path.dirname(paa), exist_ok=True)
    subprocess.run([wine, IMAGE_TO_PAA, winpath(png), winpath(paa)], env=env, check=True)


if __name__ == '__main__':
    os.makedirs(os.path.dirname(PNG), exist_ok=True)
    draw().save(PNG)
    print('wrote', PNG)
    if '--no-paa' not in sys.argv:
        to_paa(PNG, PAA)
        print('wrote', PAA, os.path.getsize(PAA), 'bytes')
