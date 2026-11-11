#!/usr/bin/env python3
"""Write the danger sign's board: an old steel sheet - rounded corners, bowed, dented, one corner
bent out - with the sign (DayZPortals/data/danger_sign_co.paa, from tools/danger_sign.py) on both
faces and bare metal round the rim. No posts. An unbinarised (MLOD)
model, one resolution LOD:

  tools/danger_board.py     writes DayZPortals/data/danger_board.p3d

The origin is the middle of the board; the sign reads from both +z and -z, 2:1 like the texture.
If the sign reads mirrored in game, run with --flip (swaps which face gets which mapping).
"""
import math, os, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, 'DayZPortals/data/danger_board.p3d')
WIDTH, HEIGHT, THICK = 2.4, 1.2, 0.05
FACE_TEX = r'dayzportals\data\danger_sign_co.paa'
FACE_MAT = r'dz\structures\signs\billboards\data\board_front.rvmat'
EDGE_TEX = r'dz\structures\signs\billboards\data\plech_generic1_co.paa'
EDGE_MAT = r'dz\structures\signs\billboards\data\board_metal.rvmat'


def lod(points, faces, resolution):
    """points: [(x, y, z)]; faces: [([(point, (u, v)) x4], texture, material)], each a quad
    listed once per side it should show from (the other winding is added)."""
    normals = [(0.0, 0.0, 0.0)]
    out = b'P3DM' + struct.pack('<II', 0x1C, 0x100)
    body_faces = b''
    nfaces = 0
    for verts, tex, mat in faces:
        for order in (verts, verts[::-1]):		# both windings: seen from either side
            body_faces += struct.pack('<I', 4)
            for p, (u, v) in order:
                body_faces += struct.pack('<IIff', p, 0, u, v)
            body_faces += struct.pack('<I', 0) + tex.encode() + b'\0' + mat.encode() + b'\0'
            nfaces += 1
    out += struct.pack('<IIII', len(points), len(normals), nfaces, 0)
    for x, y, z in points:
        out += struct.pack('<fffI', x, y, z, 0)
    for n in normals:
        out += struct.pack('<fff', *n)
    out += body_faces
    out += b'TAGG'
    out += b'\x01' + b'#EndOfFile#\0' + struct.pack('<I', 0)
    out += struct.pack('<f', resolution)
    return out


NX, NY = 48, 24			# grid cells across and down each face
CORNER = 0.15			# corner radius


def warp(x, y):
    """How far out of true the sheet is at (x, y): bowed, dented, a corner bent out."""
    w, h = WIDTH / 2, HEIGHT / 2
    bow = 0.018 * math.sin(math.pi * (x + w) / (2 * w)) * math.sin(math.pi * (y + h) / (2 * h))
    dent = -0.03 * math.exp(-((x - 0.55) ** 2 + (y + 0.22) ** 2) / 0.035)
    cx, cy = max(0.0, (x - (w - 0.55)) / 0.55), max(0.0, (y - (h - 0.45)) / 0.45)
    bend = 0.06 * (cx * cy) ** 1.5
    twist = 0.012 * (x / w) * (y / h)
    return bow + dent + bend + twist


def outline(i, j):
    """Grid point (i, j) on the face, its corners pulled onto CORNER arcs."""
    w, h = WIDTH / 2, HEIGHT / 2
    x, y = -w + 2 * w * i / NX, h - 2 * h * j / NY
    if abs(x) > w - CORNER and abs(y) > h - CORNER:
        cx, cy = math.copysign(w - CORNER, x), math.copysign(h - CORNER, y)
        dx, dy = x - cx, y - cy
        dist = math.hypot(dx, dy)
        if dist > CORNER:
            x, y = cx + dx / dist * CORNER, cy + dy / dist * CORNER
    return x, y


def board(flip):
    w, h, t = WIDTH / 2, HEIGHT / 2, THICK / 2
    pts, faces = [], []
    index = {}
    for side in (1, -1):			# +z face, then -z face
        for j in range(NY + 1):
            for i in range(NX + 1):
                x, y = outline(i, j)
                index[(side, i, j)] = len(pts)
                pts.append((x, y, warp(x, y) + side * t))

    # DayZ looks down +z with +x on the right: the -z face reads left to right as is, the
    # +z face (seen from its own side, looking back) needs u mirrored
    def uv(p, side):
        x, y = pts[p][0], pts[p][1]
        u = (x + w) / (2 * w)
        if (side > 0) != flip:
            u = 1 - u
        return (u, (h - y) / (2 * h))

    for side in (1, -1):
        for j in range(NY):
            for i in range(NX):
                quad = [index[(side, i, j)], index[(side, i + 1, j)], index[(side, i + 1, j + 1)], index[(side, i, j + 1)]]
                faces.append(([(p, uv(p, side)) for p in quad], FACE_TEX, FACE_MAT))

    # the rim: round the outline, front to back
    ring = [(i, 0) for i in range(NX)] + [(NX, j) for j in range(NY)] + [(i, NY) for i in range(NX, 0, -1)] + [(0, j) for j in range(NY, 0, -1)]
    for k in range(len(ring)):
        a, b = ring[k], ring[(k + 1) % len(ring)]
        quad = [index[(1,) + a], index[(1,) + b], index[(-1,) + b], index[(-1,) + a]]
        faces.append(([(p, e) for p, e in zip(quad, [(0, 0), (1, 0), (1, 1), (0, 1)])], EDGE_TEX, EDGE_MAT))
    return pts, faces


if __name__ == '__main__':
    pts, faces = board('--flip' in sys.argv)
    data = b'MLOD' + struct.pack('<II', 257, 1) + lod(pts, faces, 1.0)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    open(OUT, 'wb').write(data)
    print('wrote', OUT, len(data), 'bytes')
