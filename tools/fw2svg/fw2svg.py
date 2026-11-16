"""Fireworks PNG (mkTS/mkBT chunks) -> SVG. Usage: fw2svg.py in.png out.svg [--layer-order=fwd|rev] [--elem-order=fwd|rev]"""
import sys, base64, zlib, struct, math
import fwparse, os
SHIFT = os.environ.get('FW_SHIFT', '1') == '1'  # odd-width strokes on pixel centres (measured: better)
PREMUL = os.environ.get('FW_PREMUL') == '1'  # tiles are straight alpha (measured: un-premultiplying is worse)

TEXT_MODE = os.environ.get('FW_TEXT', 'live')  # live = SVG <text>; bitmap = Fireworks' cached rendering
FONT_FALLBACK = {'DejaVuSansMono': "'DejaVu Sans Mono',Consolas,monospace", 'LucidaConsole': "'Lucida Console',monospace",
                 'Courier': "'Courier New',Courier,monospace", 'Corbel': "Corbel,'Segoe UI',sans-serif"}
# Fireworks drew "Courier" with the Windows bitmap font (coure.fon), much larger than Courier New at the same size:
# its 10 pt digits are 9 px tall with a 9 px advance, i.e. Courier New at 15 px (measured on the cached bitmaps).
FONT_SCALE = {'Courier': 1.5}

def argb(v):
    v &= 0xffffffff
    return (v >> 24) & 255, (v >> 16) & 255, (v >> 8) & 255, v & 255

def css(v):
    a, r, g, b = argb(v)
    return '#%02x%02x%02x' % (r, g, b), a / 255.0

def png_bytes(w, h, rgba):
    raw = b''.join(b'\x00' + rgba[y * w * 4:(y + 1) * w * 4] for y in range(h))
    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))

class Conv:
    def __init__(self, root, tiles, elem_rev):
        self.root, self.tiles, self.elem_rev = root, tiles, elem_rev
        self.defs = []
        self.nid = 0
        self.stats = {}
    def uid(self, p):
        self.nid += 1
        return '%s%d' % (p, self.nid)
    def stat(self, k):
        self.stats[k] = self.stats.get(k, 0) + 1

    # ---- bitmaps
    def image(self, img, extra_op=1.0):
        w, h, ts = img.get('WPX'), img.get('HPX'), img.get('TSZ') or 128
        tids = [t.val for t in img.get('TIL').kids('TID')]
        nx = (w + ts - 1) // ts
        out = bytearray(w * h * 4)
        for k, tid in enumerate(tids):
            data = self.tiles.get(tid)
            if not data:
                self.stat('missing tile')
                continue
            tx, ty = (k % nx) * ts, (k // nx) * ts
            for y in range(ts):
                yy = ty + y
                if yy >= h:
                    break
                row = data[y * ts * 4:(y + 1) * ts * 4]
                for x in range(min(ts, w - tx)):
                    a, r, g, b = row[x * 4:x * 4 + 4]
                    if PREMUL and a and a < 255:
                        r, g, b = min(255, r * 255 // a), min(255, g * 255 // a), min(255, b * 255 // a)
                    o = (yy * w + tx + x) * 4
                    out[o:o + 4] = bytes((r, g, b, a))
        x = img.get('XLC', 0.0) + img.get('XOF', 0.0)
        y = img.get('YLC', 0.0) + img.get('YOF', 0.0)
        op = self.opacity(img) * extra_op
        b64 = base64.b64encode(png_bytes(w, h, bytes(out))).decode()
        self.stat('image')
        return '<image x="%g" y="%g" width="%d" height="%d"%s href="data:image/png;base64,%s"/>' % (
            x, y, w, h, '' if op >= 0.999 else ' opacity="%.3f"' % op, b64)

    def opacity(self, n):
        o = n.get('OPA')
        return 1.0 if o is None else max(0.0, min(1.0, o / 1000.0))

    # ---- paths
    def path_d(self, pth):
        d = []
        pbl = pth.get('PBL')
        for pbp in pbl.kids('PBP'):
            pts = pbp.kids('PBT')
            if not pts:
                continue
            closed = pbp.get('ISC', False)
            P = []
            for p in pts:
                x, y = p.get('XLC'), p.get('YLC')
                P.append((x, y, p.get('XPC', x), p.get('YPC', y), p.get('XSC', x), p.get('YSC', y)))
            d.append('M%.3f %.3f' % (P[0][0], P[0][1]))
            n = len(P)
            segs = n if closed else n - 1
            for i in range(segs):
                a, b = P[i], P[(i + 1) % n]
                d.append('C%.3f %.3f %.3f %.3f %.3f %.3f' % (a[4], a[5], b[2], b[3], b[0], b[1]))
            if closed:
                d.append('Z')
        return ' '.join(d)

    def fill(self, pth, pat):
        fpl = pat.get('FPL')
        if not fpl:
            return 'none', 1.0
        cat = fpl.get('CAT')
        if cat == 'fc_Solid':
            c, a = css(pat.get('FCL'))
            return c, a
        if cat == 'fc_Linear':
            stops = []
            fgv = fpl.get('FGL').get('FGV') if fpl.get('FGL') else None
            for s in (fgv.kids('FGI') if fgv else []):
                c, a = css(s.get('FGC'))
                stops.append('<stop offset="%.4f" stop-color="%s" stop-opacity="%.3f"/>' % (s.get('FGP'), c, a))
            gid = self.uid('g')
            self.defs.append('<linearGradient id="%s" gradientUnits="userSpaceOnUse" x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f">%s</linearGradient>' % (
                gid, pth.get('PSX'), pth.get('PSY'), pth.get('PEX'), pth.get('PEY'), ''.join(stops)))
            self.stat('linear')
            return 'url(#%s)' % gid, 1.0
        self.stat('fill ' + str(cat))
        c, a = css(pat.get('FCL'))
        return c, a

    def stroke(self, pat):
        bpl = pat.get('BPL')
        if not bpl:
            return None
        c, a = css(pat.get('BCL'))
        w = bpl.get('BDI', 1)
        if bpl.get('CAT') == 'bc_Pencil' and os.environ.get('FW_PENCIL1') == '1':
            w = 1
        return c, a, max(1, w), False

    def path(self, pth):
        pat = pth.get('PAT')
        d = self.path_d(pth)
        if not d:
            return ''
        f, fa = self.fill(pth, pat)
        st = self.stroke(pat)
        op = self.opacity(pth)
        attrs = ['d="%s"' % d, 'fill="%s"' % f]
        if f != 'none' and fa < 0.999:
            attrs.append('fill-opacity="%.3f"' % fa)
        stroke_el = ''
        if st:
            c, a, w, soft = st
            # Fireworks paints strokes into whole pixels: an odd width centred on integer coordinates
            # covers the pixel to the right/below, i.e. the SVG stroke shifted by half a pixel.
            sh = ' transform="translate(0.5 0.5)"' if (int(w) % 2 == 1 and SHIFT) else ''
            stroke_el = '<path d="%s" fill="none" stroke="%s" stroke-width="%g"%s%s/>' % (
                d, c, w, '' if a >= 0.999 else ' stroke-opacity="%.3f"' % a, sh)
            if soft:
                fid = self.uid('b')
                self.defs.append('<filter id="%s" x="-20%%" y="-20%%" width="140%%" height="140%%"><feGaussianBlur stdDeviation="0.6"/></filter>' % fid)
                attrs.append('filter="url(#%s)"' % fid)
        if op < 0.999:
            attrs.append('opacity="%.3f"' % op)
        if pth.get('PAT').get('FPL') and pth.get('PAT').get('FPL').get('FET') == 0:
            attrs.append('shape-rendering="crispEdges"')
        self.stat('path')
        if f == 'none':
            out = stroke_el if op >= 0.999 else '<g opacity="%.3f">%s</g>' % (op, stroke_el)
            return out
        out = '<path %s/>' % ' '.join(attrs)
        if stroke_el:
            out = '<g%s>%s%s</g>' % ('' if op >= 0.999 else ' opacity="%.3f"' % op, out.replace(' opacity="%.3f"' % op, ''), stroke_el)
        tex = self.texture(pth, pat, d)
        if tex:
            out = '<g%s>%s%s</g>' % ('' if op >= 0.999 else ' opacity="%.3f"' % op, out.replace(' opacity="%.3f"' % op, ''), tex)
        return out

    # ---- fill textures: the texture mask (8-bit, row stride TSZ) lightens the fill where it is dark:
    # a white overlay with alpha = amount (FTB, tenths of a percent) * (1 - mask/255), tiled from the
    # texture offset (TOX/TOY); both measured against the flattened image.
    def texture(self, pth, pat, d):
        fpl = pat.get('FPL')
        if not fpl or not fpl.get('FTB'):
            return ''
        txf = pat.get('TXF')
        msk = txf.get('MSK') if txf else None
        if not msk:
            return ''
        amt = fpl.get('FTB') / 1000.0
        w, h, ts = msk.get('WPX'), msk.get('HPX'), msk.get('TSZ')
        tid = msk.get('TIL').kids('TID')[0].val
        data = self.tiles.get(tid)
        if not data:
            return ''
        ox, oy = int(round(pth.get('TOX', 0.0))), int(round(pth.get('TOY', 0.0)))  # texture offset
        key = (tid, amt, ox % w, oy % h)
        if not hasattr(self, 'patterns'):
            self.patterns = {}
        pid = self.patterns.get(key)
        if not pid:
            px = bytearray(w * h * 4)
            for y in range(h):
                for x in range(w):
                    m = data[y * ts + x]
                    a = int(255 * amt * (1.0 - m / 255.0))  # truncated, as Fireworks (7/255 at 3 %)
                    px[(y * w + x) * 4:(y * w + x) * 4 + 4] = bytes((255, 255, 255, a))
            pid = self.uid('t')
            self.patterns[key] = pid
            self.defs.append('<pattern id="%s" patternUnits="userSpaceOnUse" x="%d" y="%d" width="%d" height="%d">'
                             '<image width="%d" height="%d" style="image-rendering:pixelated" href="data:image/png;base64,%s"/></pattern>' % (
                                 pid, ox % w, oy % h, w, h, w, h, base64.b64encode(png_bytes(w, h, bytes(px))).decode()))
        self.stat('texture ' + (txf.get('INM') or '?'))
        return '<path d="%s" fill="url(#%s)"/>' % (d, pid)

    def text(self, txt):
        if TEXT_MODE != 'bitmap':
            t = self.live_text(txt)
            if t is not None:
                return t
        img = txt.get('IMG')
        if img and img.get('WPX'):
            self.stat('text (cached bitmap)')
            return self.image(img, self.opacity(txt))
        self.stat('text missing')
        return ''

    def live_text(self, txt):
        """Real SVG text: one <tspan> per style run, anchored at the Fireworks baseline point (LFA, TOA)."""
        runs = [r for r in txt.kids('TFS') if r.get('TRN')]
        if not runs or any(c in r.get('TRN') for r in runs for c in '\r\n'):
            return None  # multi-line text: keep the cached bitmap
        anchor = {0: 'start', 1: 'middle', 2: 'end'}.get(txt.get('JST', 0))
        if anchor is None:
            return None  # justified
        spans = []
        for r in runs:
            col, a = css(r.get('FCL', txt.get('PAT').get('FCL') if txt.get('PAT') else -16777216))
            fam = r.get('FON') or 'sans-serif'
            st = ['font-family:%s' % FONT_FALLBACK.get(fam, "'%s'" % fam),
                  'font-size:%gpx' % (r.get('PTS', 10.0) * FONT_SCALE.get(fam, 1.0)), 'fill:%s' % col]
            if a < 0.999:
                st.append('fill-opacity:%.3f' % a)
            if r.get('BOL'):
                st.append('font-weight:bold')
            if r.get('ITL'):
                st.append('font-style:italic')
            if r.get('UND'):
                st.append('text-decoration:underline')
            k = r.get('KRN', 0.0)
            if k:
                st.append('letter-spacing:%gem' % (k / 10.0))  # Fireworks range kerning: tenths of an em (fitted)
            s = r.get('TRN').replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
            spans.append('<tspan style="%s">%s</tspan>' % (';'.join(st), s))
        attrs = 'x="%g" y="%g"' % (txt.get('LFA'), txt.get('TOA'))
        if anchor != 'start':
            attrs += ' text-anchor="%s"' % anchor
        op = self.opacity(txt)
        if op < 0.999:
            attrs += ' opacity="%.3f"' % op
        m = txt.get('MTX')
        if m:
            g = dict((c.tag, c.val) for c in m.kids())
            attrs += ' transform="matrix(%g %g %g %g %g %g)"' % (g['M00'], g['M01'], g['M10'], g['M11'], g['M20'], g['M21'])
        self.stat('text')
        return '<text %s xml:space="preserve">%s</text>' % (attrs, ''.join(spans))

    def element(self, e):
        if e.get('VIS') is False:
            self.stat('hidden')
            return ''
        if e.tag == 'PTH':
            return self.path(e)
        if e.tag == 'IMG':
            return self.image(e)
        if e.tag == 'TXT':
            return self.text(e)
        if e.tag == 'GRP':
            inner = self.elements(e.get('ELM'))
            op = self.opacity(e)
            self.stat('group')
            return '<g%s>%s</g>' % ('' if op >= 0.999 else ' opacity="%.3f"' % op, inner)
        self.stat('skip ' + e.tag)
        return ''

    def elements(self, elm):
        ks = elm.kids()
        if self.elem_rev:
            ks = list(reversed(ks))
        return '\n'.join(self.element(k) for k in ks)


def convert(src, dst, layer_rev=True, elem_rev=True):
    doc, tiles = fwparse.read_png(src)
    root = fwparse.parse(doc)
    W, H = int(root.get('WID')), int(root.get('HIT'))
    c = Conv(root, tiles, elem_rev)
    layers = root.get('LYL').kids('LAY')
    if layer_rev:
        layers = list(reversed(layers))
    body = []
    for lay in layers:
        name = lay.get('LNM')
        vis = lay.get('VIF')
        hidden = False
        if vis is not None:
            vv = [k.val for k in vis.kids() if k.typ == 'b']
            hidden = bool(vv) and not vv[0]
        cel = lay.get('CLL').kids('CEL')[0]
        if hidden or cel.get('VIS') is False:
            c.stat('hidden layer')
            body.append('<!-- layer %s hidden -->' % name)
            continue
        body.append('<g id="%s">%s</g>' % (name.replace(' ', '_'), c.elements(cel.get('ELM'))))
    svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">\n<defs>%s</defs>\n%s\n</svg>\n'
           % (W, H, W, H, '\n'.join(c.defs), '\n'.join(body)))
    open(dst, 'w', encoding='utf-8', newline='\n').write(svg)
    return c.stats


if __name__ == '__main__':
    lr = '--layer-order=rev' in sys.argv  # file order is bottom layer first
    er = '--elem-order=fwd' not in sys.argv
    print(convert(sys.argv[1], sys.argv[2], lr, er))
