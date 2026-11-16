"""Parser for the Fireworks PNG document text (mkTS chunk, zlib) and bitmap tiles (mkBT chunks).

Tagged format: TAG (3 letters) + type char + '{' ... '}':
  v = list of child items, s = uint16 big-endian length + UTF-16BE text, i = hex int,
  f = decimal float, b = bool (0/1). Other type chars are kept as raw text.
"""
import struct, zlib, sys

def read_png(path):
    b = open(path, 'rb').read()
    i = 8
    doc = None
    tiles = {}
    while i < len(b):
        n, = struct.unpack('>I', b[i:i + 4])
        t = b[i + 4:i + 8].decode('latin1')
        d = b[i + 8:i + 8 + n]
        if t == 'mkTS':
            doc = zlib.decompress(d)
        elif t == 'mkBT':
            tid, = struct.unpack('>I', d[4:8])
            p = d.find(b'\x78\x9c')
            tiles[tid] = zlib.decompressobj().decompress(d[p:]) if p >= 0 else b''
        i += 12 + n
    return doc, tiles


class Node:
    __slots__ = ('tag', 'typ', 'val')
    def __init__(self, tag, typ, val):
        self.tag, self.typ, self.val = tag, typ, val
    def kids(self, tag=None):
        if self.typ != 'v':
            return []
        return [k for k in self.val if tag is None or k.tag == tag]
    def get(self, tag, default=None):
        for k in self.kids():
            if k.tag == tag:
                return k.val if k.typ != 'v' else k
        return default
    def __repr__(self):
        return '%s%s' % (self.tag, self.typ)


def parse(d):
    sys.setrecursionlimit(100000)
    pos = 0
    def item():
        nonlocal pos
        tag = d[pos:pos + 3].decode('latin1')
        typ = chr(d[pos + 3])
        assert d[pos + 4] == ord('{'), (pos, d[pos:pos + 20])
        pos += 5
        if typ == 'v':
            kids = []
            while d[pos] != ord('}'):
                kids.append(item())
            pos += 1
            return Node(tag, typ, kids)
        if typ == 's':
            L, = struct.unpack('>H', d[pos:pos + 2])
            s = d[pos + 2:pos + 2 + 2 * L].decode('utf-16-be', 'replace')
            pos += 2 + 2 * L
            assert d[pos] == ord('}'), (pos, d[pos - 10:pos + 10])
            pos += 1
            return Node(tag, typ, s)
        end = d.index(b'}', pos)
        raw = d[pos:end].decode('latin1')
        pos = end + 1
        if typ == 'i':
            v = int(raw, 16) if raw else 0
            if v >= 0x80000000:
                v -= 0x100000000
            return Node(tag, typ, v)
        if typ == 'f':
            return Node(tag, typ, float(raw) if raw else 0.0)
        if typ == 'b':
            return Node(tag, typ, raw == '1')
        return Node(tag, typ, raw)
    root = item()
    return root


if __name__ == '__main__':
    doc, tiles = read_png(sys.argv[1])
    root = parse(doc)
    print('root', root, 'tiles', len(tiles))
    import collections
    def walk(n, depth, maxd):
        if depth > maxd:
            return
        if n.typ == 'v':
            c = collections.Counter(k.tag + k.typ for k in n.val)
            print('  ' * depth + n.tag + n.typ, dict(c) if len(c) < 14 else len(n.val))
            for k in n.val:
                if k.typ == 'v' and k.tag in ('LYL', 'LAY', 'CLL', 'CEL', 'ELM', 'FRL', 'FRM', 'PGL', 'PAG', 'GRP', 'MKB'):
                    walk(k, depth + 1, maxd)
    walk(root, 0, int(sys.argv[2]) if len(sys.argv) > 2 else 4)
