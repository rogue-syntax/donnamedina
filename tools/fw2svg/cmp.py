"""Render an SVG with Edge headless and compare it with a reference PNG (both RGBA, composited on black).
Usage: cmp.py ref.png file.svg out_prefix [scale]"""
import sys, os, struct, zlib, subprocess

def decode_png(path):
    b = open(path, 'rb').read()
    i = 8
    idat = b''
    while i < len(b):
        n, = struct.unpack('>I', b[i:i + 4])
        t = b[i + 4:i + 8]
        d = b[i + 8:i + 8 + n]
        if t == b'IHDR':
            w, h, bd, ct = struct.unpack('>IIBB', d[:10])
        elif t == b'IDAT':
            idat += d
        i += 12 + n
    assert bd == 8 and ct in (2, 6), (bd, ct)
    bpp = 4 if ct == 6 else 3
    raw = zlib.decompress(idat)
    stride = w * bpp
    out = bytearray(h * stride)
    prev = bytearray(stride)
    p = 0
    for y in range(h):
        f = raw[p]; p += 1
        line = bytearray(raw[p:p + stride]); p += stride
        if f == 1:
            for x in range(bpp, stride): line[x] = (line[x] + line[x - bpp]) & 255
        elif f == 2:
            for x in range(stride): line[x] = (line[x] + prev[x]) & 255
        elif f == 3:
            for x in range(stride): line[x] = (line[x] + ((line[x - bpp] if x >= bpp else 0) + prev[x]) // 2) & 255
        elif f == 4:
            for x in range(stride):
                a = line[x - bpp] if x >= bpp else 0
                bb = prev[x]; c = prev[x - bpp] if x >= bpp else 0
                pa, pb, pc = abs(bb - c), abs(a - c), abs(a + bb - 2 * c)
                pr = a if pa <= pb and pa <= pc else (bb if pb <= pc else c)
                line[x] = (line[x] + pr) & 255
        out[y * stride:(y + 1) * stride] = line
        prev = line
    if bpp == 3:
        o4 = bytearray(w * h * 4)
        for k in range(w * h):
            o4[k * 4:k * 4 + 3] = out[k * 3:k * 3 + 3]; o4[k * 4 + 3] = 255
        out = o4
    return w, h, bytes(out)

def write_png(path, w, h, rgba):
    raw = b''.join(b'\x00' + rgba[y * w * 4:(y + 1) * w * 4] for y in range(h))
    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)) +
                           chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))

def on_black(rgba):
    out = bytearray(len(rgba))
    for k in range(0, len(rgba), 4):
        a = rgba[k + 3]
        out[k] = rgba[k] * a // 255; out[k + 1] = rgba[k + 1] * a // 255; out[k + 2] = rgba[k + 2] * a // 255; out[k + 3] = 255
    return out

def render(svg, png, w, h, scale=1):
    edge = r'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe'
    html = png[:-4] + '_wrap.html'  # next to the output, not the SVG
    open(html, 'w').write('<html><body style="margin:0;background:#000"><img src="file:///%s" width="%d" height="%d" style="display:block"></body></html>'
                          % (svg.replace(chr(92), '/'), w * scale, h * scale))
    subprocess.run([edge, '--headless=new', '--disable-gpu', '--hide-scrollbars', '--force-device-scale-factor=1',
                    '--screenshot=' + png, '--window-size=%d,%d' % (w * scale, h * scale),
                    'file:///' + html.replace('\\', '/')], capture_output=True, timeout=60)

if __name__ == '__main__':
    ref, svg, pre = sys.argv[1], sys.argv[2], sys.argv[3]
    w, h, a = decode_png(ref)
    render(os.path.abspath(svg), os.path.abspath(pre + '_render.png'), w, h)
    w2, h2, b = decode_png(pre + '_render.png')
    assert (w, h) == (w2, h2), (w, h, w2, h2)
    A = on_black(a); B = on_black(b)
    tot = 0; big = 0
    diff = bytearray(len(A))
    for k in range(0, len(A), 4):
        d = (abs(A[k] - B[k]) + abs(A[k + 1] - B[k + 1]) + abs(A[k + 2] - B[k + 2])) // 3
        tot += d
        if d > 40: big += 1
        v = min(255, d * 3)
        diff[k] = v; diff[k + 1] = v; diff[k + 2] = v; diff[k + 3] = 255
    n = w * h
    print('mean abs diff %.2f / 255, pixels off by >40: %.2f %%' % (tot / n, 100.0 * big / n))
    write_png(pre + '_diff.png', w, h, bytes(diff))
    write_png(pre + '_ref.png', w, h, bytes(A))
