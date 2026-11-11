#!/usr/bin/env python3
"""Round-trip the gate's look (DZP_Layout.Installation) through the DayZ Editor.

  tools/layout.py import <file>  read an editor save (.dze, e.g. teleporter.dze) or an Object
                                 Spawner export (.json) and rewrite Installation() with every piece
                                 within RADIUS m of the anchor, measured from it
  tools/layout.py export [x z y] write the built gate as Object Spawner JSON into the editor's
                                 folder, the anchor at x z on ground height y (default: the
                                 Balota clearing on Chernarus the test mission uses)

The anchor is the radio panel (Land_Radio_PanelPAS, or its bare .p3d): DZP_Gate is that
model, it's what players tune. With several panels in a save, the one nearest the saved
camera is the anchor - leave the camera looking at the build before saving. The jump spot
(DZP_Layout.CHAMBER) is the middle of the gantries (the decon showers) when there are any, and
the danger sign goes on the fence panel nearest the radio panel.
A bare file name is looked up in the editor's folder.
"""
import json, math, os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LAYOUT = os.path.join(ROOT, 'DayZPortals/Scripts/4_World/DayZPortals/DZP_Layout.c')
STEAM = os.environ.get('STEAM', os.path.expanduser('~/.steam/debian-installation'))
EDITOR_DIR = os.path.join(STEAM, 'steamapps/compatdata/221100/pfx/drive_c/users/steamuser/Documents/DayZ/Editor')
EXPORT_FILE = os.path.join(EDITOR_DIR, 'pereval_gate.json')

ANCHORS = ('land_radio_panelpas', r'dz\structures\specific\radio\radio_panelpas.p3d')
GANTRY = ('staticobj_misc_deconshower_large', r'dz\structures_bliss\military\misc\misc_deconshower_large.p3d')
# the danger sign hangs on the fence panel nearest the radio panel, on its side, where whoever
# is tuning can read it
SIGN = 'DZP_DangerSign'
FENCES = ('staticobj_wall_fen4_4',)
SIGN_LIFT = 0.85			# board centre above the fence's middle (Wall_Fen4_4 is 3.4 m tall)
SIGN_OFF = 0.135			# out from the fence's middle: half its 0.21 m, plus half the board
RADIUS = 30.0
# DZP_Gate (an item) shows the panel model turned round from the Land_Radio_PanelPAS the editor
# places, so the layout is measured from the panel's back
ANCHOR_TURN = 180.0
# the Balota clearing (portals.chernarusplus gate A) and the ground height there
DEFAULT_SPOT = (4850.0, 8.1767, 2440.0)

PART = re.compile(r'parts\.Insert\(new DZP_DecorPart\("([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*([0-9.]+)\)\);')


def norm(name):
    return name.replace('/', '\\').lower()


# --- the editor's files ------------------------------------------------------------------

def read_dze(path):
    """Objects (type, pos, ypr, scale) and the camera position from a binary editor save
    (Dabs Framework EditorSaveData: strings are int length + bytes, vectors int 3 + 3 floats)."""
    d = open(path, 'rb').read()

    def string(i):
        n = struct.unpack_from('<i', d, i)[0]
        if not 0 <= n < 512 or i + 4 + n > len(d):
            return None, i
        s = d[i + 4:i + 4 + n]
        if any(c < 32 or c > 126 for c in s):
            return None, i
        return s.decode(), i + 4 + n

    if string(0)[0] != 'EditorBinned':
        sys.exit('%s is not a binary editor save' % path)
    _, i = string(0)
    i += 4                                  # version
    _, i = string(i)                        # map name
    camera = struct.unpack_from('<3f', d, i + 4)
    objects = []
    # each object record starts Type, DisplayName, Position, Orientation, Scale; the rest
    # (flags, parameters...) varies, so scan for that shape rather than walk every field
    while i < len(d) - 40:
        typ, j = string(i)
        if typ and len(typ) > 3:
            _, k = string(j)
            if k != j and k + 36 <= len(d) and struct.unpack_from('<i', d, k)[0] == 3 and struct.unpack_from('<i', d, k + 16)[0] == 3:
                pos = struct.unpack_from('<3f', d, k + 4)
                ypr = struct.unpack_from('<3f', d, k + 20)
                scale = struct.unpack_from('<f', d, k + 32)[0]
                if 0.01 < scale < 100 and all(abs(v) < 30000 for v in pos + ypr):
                    objects.append((typ, pos, ypr, scale))
                    i = k + 36
                    continue
        i += 1
    return objects, camera


def read_json(path):
    objects = [(o['name'], tuple(o['pos']), tuple(o['ypr']), o.get('scale') or 1) for o in json.load(open(path))['Objects']]
    return objects, None


# --- DZP_Layout.c --------------------------------------------------------------------------

def built_block(src):
    start = src.index('static void Installation(')
    open_ = src.index('{', start)
    close = src.index('\n\t}', open_)
    return open_ + 1, close


def parse_parts(src):
    a, b = built_block(src)
    parts = []
    for m in PART.finditer(src[a:b]):
        typ, pos, ori, scale = m.groups()
        parts.append((typ.replace('\\\\', '\\'), tuple(float(v) for v in pos.split()), tuple(float(v) for v in ori.split()), float(scale)))
    return parts


# DayZ: yaw is clockwise from +z (north) seen from above; model +x is right, +z forward
def to_world(origin, yaw, p):
    s, c = math.sin(math.radians(yaw)), math.cos(math.radians(yaw))
    return (origin[0] + p[0] * c + p[2] * s, origin[1] + p[1], origin[2] - p[0] * s + p[2] * c)


def to_local(origin, yaw, w):
    s, c = math.sin(math.radians(yaw)), math.cos(math.radians(yaw))
    dx, dy, dz = w[0] - origin[0], w[1] - origin[1], w[2] - origin[2]
    return (dx * c - dz * s, dy, dx * s + dz * c)


def wrap(deg):
    deg = (deg + 180) % 360 - 180
    return 0.0 if abs(deg) < 1e-6 else deg


def fmt(v):
    s = ('%.3f' % v).rstrip('0').rstrip('.')
    return '0' if s in ('-0', '') else s


def vec(v):
    return ' '.join(fmt(x) for x in v)


def import_(path):
    if os.sep not in path and not os.path.exists(path):
        path = os.path.join(EDITOR_DIR, path)
        if not os.path.exists(path) and os.path.exists(path + '.dze'):
            path += '.dze'
    objects, camera = read_dze(path) if path.endswith('.dze') else read_json(path)

    anchors = [o for o in objects if norm(o[0]) in ANCHORS]
    if not anchors:
        sys.exit('no radio panel (Land_Radio_PanelPAS) in %s: place one, it is the gate itself' % path)
    if len(anchors) > 1 and not camera:
        sys.exit('%d radio panels in %s and no camera to pick one by' % (len(anchors), path))
    if camera:
        anchors.sort(key=lambda o: (o[1][0] - camera[0]) ** 2 + (o[1][2] - camera[2]) ** 2)
    anchor = anchors[0]
    origin, yaw = anchor[1], anchor[2][0] + ANCHOR_TURN

    lines, gantries, fences, signs, skipped = [], [], [], [], 0
    for typ, pos, ypr, scale in objects:
        if (typ, pos) == anchor[:2]:
            continue
        if math.hypot(pos[0] - origin[0], pos[2] - origin[2]) > RADIUS:
            skipped += 1
            continue
        local = to_local(origin, yaw, pos)
        ori = (wrap(ypr[0] - yaw), wrap(ypr[1]), wrap(ypr[2]))
        if norm(typ) in GANTRY:
            gantries.append(local)
        if norm(typ) in FENCES:
            fences.append((local, ori[0]))
        if typ == SIGN:
            continue			# placed below
        lines.append('\t\tparts.Insert(new DZP_DecorPart("%s", "%s", "%s", %s));' % (typ.replace('\\', '\\\\'), vec(local), vec(ori), fmt(scale)))

    if fences:
        (fx, fy, fz), fyaw = min(fences, key=lambda f: f[0][0] ** 2 + f[0][2] ** 2)
        # which side of the fence the panel (the origin) is on, in the fence's own frame
        side = 1 if to_local((fx, fy, fz), fyaw, (0, 0, 0))[2] >= 0 else -1
        at = to_world((fx, fy, fz), fyaw, (0, SIGN_LIFT, side * SIGN_OFF))
        signs.append('\t\tparts.Insert(new DZP_DecorPart("%s", "%s", "%s", 1));' % (SIGN, vec(at), vec((wrap(fyaw), 0, 0))))

    src = open(LAYOUT).read()
    a, b = built_block(src)
    src = src[:a] + '\n' + '\n'.join(lines + signs) + src[b:]
    msg = ''
    if gantries:
        chamber = (sum(g[0] for g in gantries) / len(gantries), 0, sum(g[2] for g in gantries) / len(gantries))
        src = re.sub(r'static const vector CHAMBER = "[^"]+"', 'static const vector CHAMBER = "%s"' % vec(chamber), src)
        msg = '; CHAMBER = %s (middle of %d gantries)' % (vec(chamber), len(gantries))
    open(LAYOUT, 'w').write(src)
    print('anchor %s at %s; Installation() now has %d pieces, %d further than %g m left out%s'
          % (anchor[0], vec(origin), len(lines), skipped, RADIUS, msg))


def export(spot):
    parts = parse_parts(open(LAYOUT).read())
    objects = [{'name': 'Land_Radio_PanelPAS', 'pos': list(spot), 'ypr': [0, 0, 0], 'scale': 1, 'enableCEPersistency': 0}]
    for typ, pos, ori, scale in parts:
        objects.append({'name': typ, 'pos': list(to_world(spot, ANCHOR_TURN, pos)), 'ypr': [wrap(ori[0] + ANCHOR_TURN), ori[1], ori[2]], 'scale': scale, 'enableCEPersistency': 0})
    os.makedirs(EDITOR_DIR, exist_ok=True)
    with open(EXPORT_FILE, 'w') as f:
        json.dump({'Objects': objects}, f, indent=1)
    print('wrote %d pieces + the anchor to %s' % (len(parts), EXPORT_FILE))


if __name__ == '__main__':
    if len(sys.argv) >= 2 and sys.argv[1] == 'export':
        spot = DEFAULT_SPOT
        if len(sys.argv) == 5:
            spot = (float(sys.argv[2]), float(sys.argv[4]), float(sys.argv[3]))
        export(spot)
    elif len(sys.argv) == 3 and sys.argv[1] == 'import':
        import_(sys.argv[2])
    else:
        sys.exit(__doc__)
