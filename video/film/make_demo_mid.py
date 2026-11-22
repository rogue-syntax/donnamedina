# Writes video/film/demo.mid: a short test part for the film tool (4 bars, 120 BPM, 4/4):
# a bass line, held chords, a melody, and a mod-wheel (CC1) sweep up and back.
# Run from the project root:  python video/film/make_demo_mid.py
import os
import struct

TPQ = 480


def vlq(n):
    out = [n & 0x7F]
    n >>= 7
    while n:
        out.insert(0, 0x80 | (n & 0x7F))
        n >>= 7
    return bytes(out)


events = []  # (tick, order, bytes); order: note-offs before note-ons at the same tick


def note(beat, length, pitch, vel):
    on = int(beat * TPQ)
    off = int((beat + length) * TPQ)
    events.append((on, 1, bytes([0x90, pitch, vel])))
    events.append((off, 0, bytes([0x80, pitch, 0])))


bass = [36, 36, 43, 41]  # one note per beat pair, per bar
for bar in range(4):
    for k in range(4):
        note(bar * 4 + k, 0.45, bass[bar], 100)
chords = [(60, 64, 67), (60, 64, 67), (55, 59, 62), (53, 57, 60)]
for bar, ch in enumerate(chords):
    for p in ch:
        note(bar * 4, 3.8, p, 70)
melody = [(8, 0.5, 72), (8.5, 0.5, 74), (9, 1, 76), (10, 0.5, 79), (10.5, 0.5, 77), (11, 1, 76),
          (12, 0.5, 74), (12.5, 0.5, 72), (13, 2.5, 71)]
for b, l, p in melody:
    note(b, l, p, 90)
for i in range(65):  # CC1: 0 -> 127 over bars 1-2, back to 0 over bars 3-4
    t = i / 64.0
    v = int(round(127 * (2 * t if t < 0.5 else 2 - 2 * t)))
    events.append((int(t * 16 * TPQ), 2, bytes([0xB0, 1, v])))

events.sort(key=lambda e: (e[0], e[1]))
track = bytearray()
track += vlq(0) + b'\xFF\x03' + vlq(4) + b'Demo'
track += vlq(0) + b'\xFF\x51\x03' + struct.pack('>I', 500000)[1:]  # 120 BPM
track += vlq(0) + b'\xFF\x58\x04' + bytes([4, 2, 24, 8])         # 4/4
last = 0
for tick, _, data in events:
    track += vlq(tick - last) + data
    last = tick
track += vlq(0) + b'\xFF\x2F\x00'

path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'demo.mid')
with open(path, 'wb') as f:
    f.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, TPQ))
    f.write(b'MTrk' + struct.pack('>I', len(track)) + bytes(track))
print('wrote', path, len(events), 'events')
