"""blockmap: block outlines and PCB trace pixels from the panel SVG, for the editor's block hover effect.

    python tools/blockmap/blockmap.py [fwrksrc/svg/bkg8_donnaMedina3.svg] [source/blockMap.h]

Reads the Affinity export of the panel (viewBox scaled to the 1248x542 window) and writes a header with
  - kBlocks: the outline (stroke centre line) of each control block, found as the closed paths stroked
    with the frame cyan #04bae8: the 16 tall blocks of the MAIN OSC / MOD OSC / GLOBAL row (195 px) and
    the 6 ENV/LFO blocks (100 px), named left to right;
  - kTrace: the pixels of the faint PCB traces, from the embedded bitmaps placed with <use> (the main
    osc sheet and the mod block strip, repeated per block), in window coordinates, without the pixels
    hidden by solid shapes drawn above them (knob wells etc.).
Run it again when the blocks or traces move in the art.
"""
import base64, io, math, re, struct, sys, zlib
import xml.etree.ElementTree as ET

SRC = sys.argv[1] if len(sys.argv) > 1 else 'fwrksrc/svg/bkg8_donnaMedina3.svg'
DST = sys.argv[2] if len(sys.argv) > 2 else 'source/blockMap.h'
WIN_W, WIN_H = 1248, 542
NS = '{http://www.w3.org/2000/svg}'
XL = '{http://www.w3.org/1999/xlink}href'

TOP_NAMES = ['OSC', 'WIDTH', 'TUNE', 'FINE', 'OCT', 'INT',
             'MOD OSC/TYPE', 'MOD GAIN', 'MOD WIDTH', 'MOD TUNE', 'MOD FINE', 'MOD OCT', 'MOD INT', 'MOD BEAT',
             'GLOBAL GAIN', 'GLOBAL FILTER']
ENV_NAMES = ['AMP ENV', 'ENV TWO', 'ENV THREE', 'LFO ONE', 'LFO TWO', 'LFO THREE']


# ---- geometry ---------------------------------------------------------------------------------
def mat_mul(a, b):
    return (a[0]*b[0]+a[2]*b[1], a[1]*b[0]+a[3]*b[1], a[0]*b[2]+a[2]*b[3], a[1]*b[2]+a[3]*b[3],
            a[0]*b[4]+a[2]*b[5]+a[4], a[1]*b[4]+a[3]*b[5]+a[5])

NUM = r'[-+]?(?:\d*\.\d+|\d+\.?)(?:[eE][-+]?\d+)?'

def parse_tf(t):
    m = (1, 0, 0, 1, 0, 0)
    for name, args in re.findall(r'(\w+)\s*\(([^)]*)\)', t or ''):
        v = [float(x) for x in re.findall(NUM, args)]
        if name == 'matrix':
            n = tuple(v)
        elif name == 'translate':
            n = (1, 0, 0, 1, v[0], v[1] if len(v) > 1 else 0)
        elif name == 'scale':
            n = (v[0], 0, 0, v[1] if len(v) > 1 else v[0], 0, 0)
        elif name == 'rotate':
            a = math.radians(v[0]); c, s = math.cos(a), math.sin(a)
            n = (c, s, -s, c, 0, 0)
            if len(v) == 3:
                n = mat_mul(mat_mul((1, 0, 0, 1, v[1], v[2]), n), (1, 0, 0, 1, -v[1], -v[2]))
        else:
            continue
        m = mat_mul(m, n)
    return m

def ap(m, x, y):
    return (m[0]*x + m[2]*y + m[4], m[1]*x + m[3]*y + m[5])

def subpaths(d):
    """Flattened subpaths [(points, closed)] of an SVG path (curves sampled)."""
    toks = re.findall(r'[MmLlHhVvCcSsQqAaZz]|' + NUM, d)
    i = 0; cmd = 'M'; x = y = sx = sy = 0.0; out = []; cur = []; lc = None
    def num():
        nonlocal i
        i += 1
        return float(toks[i - 1])
    def bez(p1, p2, p3, n=10):
        p0 = (x, y)
        for k in range(1, n + 1):
            t = k / n; u = 1 - t
            cur.append((u**3*p0[0] + 3*u*u*t*p1[0] + 3*u*t*t*p2[0] + t**3*p3[0],
                        u**3*p0[1] + 3*u*u*t*p1[1] + 3*u*t*t*p2[1] + t**3*p3[1]))
    while i < len(toks):
        if toks[i].isalpha():
            cmd = toks[i]; i += 1
            if cmd in 'Zz':
                if cur:
                    out.append((cur, True)); cur = []
                x, y = sx, sy; lc = None
                continue
        rel = cmd.islower(); C = cmd.upper()
        ox, oy = (x, y) if rel else (0.0, 0.0)
        if C == 'M':
            if cur:
                out.append((cur, False))
            x, y = num() + ox, num() + oy; sx, sy = x, y; cur = [(x, y)]
            cmd = 'l' if rel else 'L'; lc = None
        elif C == 'L':
            x, y = num() + ox, num() + oy; cur.append((x, y)); lc = None
        elif C == 'H':
            x = num() + ox; cur.append((x, y)); lc = None
        elif C == 'V':
            y = num() + oy; cur.append((x, y)); lc = None
        elif C == 'C':
            a = [num() for _ in range(6)]
            p1, p2, p3 = (a[0]+ox, a[1]+oy), (a[2]+ox, a[3]+oy), (a[4]+ox, a[5]+oy)
            bez(p1, p2, p3); lc = p2; x, y = p3
        elif C == 'S':
            a = [num() for _ in range(4)]
            p1 = (2*x - lc[0], 2*y - lc[1]) if lc else (x, y)
            p2, p3 = (a[0]+ox, a[1]+oy), (a[2]+ox, a[3]+oy)
            bez(p1, p2, p3); lc = p2; x, y = p3
        elif C == 'Q':
            a = [num() for _ in range(4)]
            q, p3 = (a[0]+ox, a[1]+oy), (a[2]+ox, a[3]+oy)
            bez((x + 2/3*(q[0]-x), y + 2/3*(q[1]-y)), (p3[0] + 2/3*(q[0]-p3[0]), p3[1] + 2/3*(q[1]-p3[1])), p3)
            lc = None; x, y = p3
        elif C == 'A':
            a = [num() for _ in range(7)]
            x, y = a[5] + ox, a[6] + oy; cur.append((x, y)); lc = None
        else:
            i += 1
    if cur:
        out.append((cur, False))
    return out

def inside(poly, px, py):
    c = False
    j = len(poly) - 1
    for k in range(len(poly)):
        xi, yi = poly[k]; xj, yj = poly[j]
        if (yi > py) != (yj > py) and px < (xj - xi) * (py - yi) / (yj - yi) + xi:
            c = not c
        j = k
    return c


# ---- PNG decode (8-bit RGBA / RGB, non-interlaced: what Affinity embeds) ------------------------
def png_rgba(data):
    assert data[:8] == b'\x89PNG\r\n\x1a\n'
    pos = 8; idat = b''; w = h = ct = 0
    while pos < len(data):
        n, typ = struct.unpack('>I4s', data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]; pos += 12 + n
        if typ == b'IHDR':
            w, h, bd, ct, _, _, il = struct.unpack('>IIBBBBB', body)
            assert bd == 8 and il == 0 and ct in (2, 6), (bd, ct, il)
        elif typ == b'IDAT':
            idat += body
    bpp = 4 if ct == 6 else 3
    raw = zlib.decompress(idat); stride = w * bpp
    rows = []; prev = bytearray(stride); p = 0
    for _ in range(h):
        f = raw[p]; line = bytearray(raw[p + 1:p + 1 + stride]); p += 1 + stride
        for k in range(stride):
            a = line[k - bpp] if k >= bpp else 0; b = prev[k]; c = prev[k - bpp] if k >= bpp else 0
            if f == 1: line[k] = (line[k] + a) & 255
            elif f == 2: line[k] = (line[k] + b) & 255
            elif f == 3: line[k] = (line[k] + ((a + b) >> 1)) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2*c)
                line[k] = (line[k] + (a if pa <= pb and pa <= pc else (b if pb <= pc else c))) & 255
        rows.append(line); prev = line
    alpha = [[(r[x*bpp + 3] if bpp == 4 else 255) for x in range(w)] for r in rows]
    return w, h, alpha


# ---- walk the document --------------------------------------------------------------------------
root = ET.parse(SRC).getroot()
vb = [float(v) for v in root.get('viewBox').split()]
SC = WIN_W / vb[2]
images = {}
for el in root.iter(NS + 'image'):
    href = el.get(XL) or el.get('href') or ''
    m = re.match(r'data:image/png;base64,(.*)', href, re.S)
    if m:
        images[el.get('id')] = png_rgba(base64.b64decode(m.group(1)))

items = []  # document order: ('shape', style, subpaths) / ('image', id, matrix)

def style_of(el, inh):
    st = dict(inh)
    for k in ('fill', 'stroke', 'fill-opacity'):
        if el.get(k) is not None:
            st[k] = el.get(k)
    for kv in (el.get('style') or '').split(';'):
        if ':' in kv:
            k, v = kv.split(':', 1); st[k.strip()] = v.strip()
    if el.get('opacity') is not None:
        st['op'] = st.get('op', 1.0) * float(el.get('opacity'))
    return st

def walk(el, m, inh, in_defs):
    tag = el.tag.replace(NS, '')
    if tag == 'defs':
        return
    m = mat_mul(m, parse_tf(el.get('transform')))
    st = style_of(el, inh)
    if tag in ('path', 'rect', 'circle', 'ellipse'):
        if tag == 'path':
            subs = subpaths(el.get('d'))
        elif tag == 'rect':
            x0, y0 = float(el.get('x', 0)), float(el.get('y', 0)); w, h = float(el.get('width')), float(el.get('height'))
            subs = [([(x0, y0), (x0 + w, y0), (x0 + w, y0 + h), (x0, y0 + h)], True)]
        else:
            cx, cy = float(el.get('cx', 0)), float(el.get('cy', 0))
            rx = float(el.get('r') or el.get('rx')); ry = float(el.get('r') or el.get('ry'))
            subs = [([(cx + rx*math.cos(k*math.pi/16), cy + ry*math.sin(k*math.pi/16)) for k in range(32)], True)]
        subs = [([(px*SC, py*SC) for px, py in (ap(m, *p) for p in pts)], cl) for pts, cl in subs]
        items.append(('shape', st, subs))
    elif tag == 'use':
        ref = (el.get(XL) or el.get('href') or '')[1:]
        if ref in images:
            items.append(('image', ref, mat_mul(m, (1, 0, 0, 1, float(el.get('x', 0)), float(el.get('y', 0))))))
    for ch in el:
        walk(ch, m, st, in_defs)

walk(root, (1, 0, 0, 1, -vb[0], -vb[1]), {}, False)


# ---- blocks -------------------------------------------------------------------------------------
def clean(pts):
    if len(pts) > 1 and abs(pts[0][0] - pts[-1][0]) < 0.05 and abs(pts[0][1] - pts[-1][1]) < 0.05:
        pts = pts[:-1]
    out = []
    for p in pts:
        if not out or abs(p[0] - out[-1][0]) > 0.05 or abs(p[1] - out[-1][1]) > 0.05:
            out.append(p)
    k = 0  # drop collinear points
    while k < len(out) and len(out) > 3:
        a, b, c = out[k - 1], out[k], out[(k + 1) % len(out)]
        if abs((b[0]-a[0])*(c[1]-a[1]) - (b[1]-a[1])*(c[0]-a[0])) < 0.05:
            out.pop(k)
        else:
            k += 1
    return out

top, env = [], []
for kind, st, subs in items:
    if kind != 'shape' or st.get('stroke', '').lower() != '#04bae8' or st.get('op', 1.0) < 0.5:
        continue
    for pts, closed in subs:
        if not closed:
            continue
        xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
        w, h = max(xs) - min(xs), max(ys) - min(ys)
        if 50 < w < 100 and 190 < h < 200:
            top.append(clean(pts))
        elif 130 < w < 150 and 95 < h < 105:
            env.append(clean(pts))
top.sort(key=lambda p: min(q[0] for q in p)); env.sort(key=lambda p: min(q[0] for q in p))
assert len(top) == len(TOP_NAMES) and len(env) == len(ENV_NAMES), (len(top), len(env))
blocks = [(n, 0, p) for n, p in zip(TOP_NAMES, top)] + [(n, 1, p) for n, p in zip(ENV_NAMES, env)]


# ---- trace pixels -------------------------------------------------------------------------------
occluders = []  # (index in items, polygon, bbox): solid fills drawn above the trace sheets
for idx, (kind, st, subs) in enumerate(items):
    if kind != 'shape':
        continue
    fill = st.get('fill', '#000')
    if fill == 'none' or fill.startswith('url(') or st.get('op', 1.0) < 0.5 or float(st.get('fill-opacity', 1)) < 0.5:
        continue
    for pts, closed in subs:
        if len(pts) >= 3:
            xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
            occluders.append((idx, pts, (min(xs), min(ys), max(xs), max(ys))))

trace = {}
for idx, (kind, ref, m) in enumerate(items):
    if kind != 'image':
        continue
    w, h, alpha = images[ref]
    if h < 100:  # the trace sheets are the tall bitmaps (418x167 main osc, 60x162 per mod block)
        continue
    later = [o for o in occluders if o[0] > idx]
    for v in range(h):
        for u in range(w):
            a = alpha[v][u]
            if a == 0:
                continue
            wx, wy = ap(m, u + 0.5, v + 0.5)
            wx *= SC; wy *= SC
            if any(b[0] <= wx <= b[2] and b[1] <= wy <= b[3] and inside(poly, wx, wy) for _, poly, b in later):
                continue
            key = (int(math.floor(wx)), int(math.floor(wy)))
            if 0 <= key[0] < WIN_W and 0 <= key[1] < WIN_H:
                trace[key] = max(trace.get(key, 0), a)


# ---- header -------------------------------------------------------------------------------------
L = []
L.append('// Generated by tools/blockmap/blockmap.py from %s. Do not edit; run the script again.' % SRC.replace('\\', '/'))
L.append('// Window coordinates (1248x542). Used by NVGView::drawBlockFx (block hover effect).')
L.append('#pragma once')
L.append('')
L.append('namespace blockMap {')
L.append('')
L.append('struct Pt { float x, y; };')
L.append('constexpr int kMaxPts = %d;' % max(len(p) for _, _, p in blocks))
L.append('// row 0: MAIN OSC / MOD OSC / GLOBAL blocks, row 1: ENV/LFO blocks. Outline = stroke centre line.')
L.append('struct Block { const char* name; int row; int n; Pt p[kMaxPts]; };')
L.append('constexpr int kBlockCount = %d;' % len(blocks))
L.append('constexpr Block kBlocks[kBlockCount] = {')
for n, row, p in blocks:
    L.append('    {"%s", %d, %d, {%s}},' % (n, row, len(p), ', '.join('{%.2ff, %.2ff}' % q for q in p)))
L.append('};')
L.append('')
L.append('// PCB trace pixels of the art (x, y, alpha 0..255 as painted, max about 100).')
L.append('struct TracePx { unsigned short x, y; unsigned char a; };')
keys = sorted(trace, key=lambda k: (k[1], k[0]))
L.append('constexpr int kTraceCount = %d;' % len(keys))
L.append('constexpr TracePx kTrace[kTraceCount] = {')
row = []
for k in keys:
    row.append('{%d,%d,%d}' % (k[0], k[1], trace[k]))
    if len(row) == 12:
        L.append('    ' + ','.join(row) + ','); row = []
if row:
    L.append('    ' + ','.join(row) + ',')
L.append('};')
L.append('')
L.append('}  // namespace blockMap')
io.open(DST, 'w', encoding='utf-8', newline='\n').write('\n'.join(L) + '\n')
print('%d blocks, %d trace pixels (%d sheets) -> %s' % (len(blocks), len(keys), sum(1 for i in items if i[0] == 'image' and images[i[1]][1] >= 100), DST))
