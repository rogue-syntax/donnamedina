#pragma once
// Standard MIDI File reader for the film tool (2026-11-21): formats 0 and 1, ticks-per-quarter
// timing (SMPTE timing is rejected), running status, tempo map and the first time signature.
// Channel events from all tracks are merged and timed in seconds through the tempo map.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace midifile {

struct Event {
	int64_t tick = 0;
	double sec = 0.0;
	int track = 0;
	uint8_t status = 0;  // 0x80..0xEF (channel message; low nibble = channel)
	uint8_t d1 = 0, d2 = 0;
	int type() const { return status & 0xF0; }
	int channel() const { return status & 0x0F; }
	bool isNoteOn() const { return type() == 0x90 && d2 > 0; }
	bool isNoteOff() const { return type() == 0x80 || (type() == 0x90 && d2 == 0); }
};

struct Tempo {
	int64_t tick = 0;
	double usPerQuarter = 500000.0;  // 120 BPM
	double sec = 0.0;                // when it starts
};

struct File {
	bool ok = false;
	std::string error;
	int format = 0, tracks = 0, division = 480;  // ticks per quarter note
	int timeSigNum = 4, timeSigDen = 4;
	std::vector<Event> events;   // sorted by time
	std::vector<Tempo> tempos;   // sorted by tick, first at tick 0
	std::vector<std::string> trackNames;
	double lengthSec = 0.0;      // last event

	double secAtTick(int64_t tick) const {
		const Tempo* t = &tempos[0];
		for (const Tempo& x : tempos)
			if (x.tick <= tick) t = &x;
		return t->sec + double(tick - t->tick) * t->usPerQuarter * 1e-6 / division;
	}
	// position in quarter notes (the host's project time in music)
	double quartersAtSec(double sec) const {
		const Tempo* t = &tempos[0];
		for (const Tempo& x : tempos)
			if (x.sec <= sec) t = &x;
		return (double(t->tick) + (sec - t->sec) / (t->usPerQuarter * 1e-6 / division)) / division;
	}
	double bpmAtSec(double sec) const {
		const Tempo* t = &tempos[0];
		for (const Tempo& x : tempos)
			if (x.sec <= sec) t = &x;
		return 60.0e6 / t->usPerQuarter;
	}
};

namespace detail {
struct Reader {
	const std::vector<uint8_t>& b;
	size_t p = 0, end = 0;
	bool bad = false;
	explicit Reader(const std::vector<uint8_t>& d) : b(d), end(d.size()) {}
	uint8_t u8() { if (p >= end) { bad = true; return 0; } return b[p++]; }
	uint32_t be(int n) { uint32_t v = 0; for (int i = 0; i < n; i++) v = (v << 8) | u8(); return v; }
	uint32_t vlq() {
		uint32_t v = 0;
		for (int i = 0; i < 4; i++) {
			const uint8_t c = u8();
			v = (v << 7) | (c & 0x7F);
			if (!(c & 0x80)) return v;
		}
		bad = true;
		return v;
	}
};
} // namespace detail

inline File read(const std::wstring& path) {
	File f;
	std::vector<uint8_t> data;
	if (FILE* fp = _wfopen(path.c_str(), L"rb")) {
		uint8_t buf[65536];
		size_t n;
		while ((n = std::fread(buf, 1, sizeof(buf), fp)) > 0) data.insert(data.end(), buf, buf + n);
		std::fclose(fp);
	} else {
		f.error = "cannot open the MIDI file";
		return f;
	}
	detail::Reader r(data);
	if (r.be(4) != 0x4D546864u || r.be(4) < 6) { f.error = "not a MIDI file (no MThd header)"; return f; }
	f.format = int(r.be(2));
	f.tracks = int(r.be(2));
	const uint32_t div = r.be(2);
	if (div & 0x8000) { f.error = "SMPTE timing is not supported (export with ticks per quarter note)"; return f; }
	f.division = int(div);
	if (f.division <= 0 || f.format > 1) { f.error = "unsupported MIDI file (format " + std::to_string(f.format) + ")"; return f; }
	r.p = 14;
	std::vector<Tempo> tempos;
	bool timeSigSeen = false;
	for (int t = 0; t < f.tracks && !r.bad; t++) {
		const uint32_t id = r.be(4), len = r.be(4);
		const size_t trackEnd = r.p + len;
		if (trackEnd > data.size()) { r.bad = true; break; }
		f.trackNames.push_back("");
		if (id != 0x4D54726Bu) { r.p = trackEnd; continue; }  // not MTrk: skip
		r.end = trackEnd;
		int64_t tick = 0;
		uint8_t running = 0;
		while (r.p < trackEnd && !r.bad) {
			tick += r.vlq();
			uint8_t st = r.u8();
			if (st == 0xFF) {  // meta
				const uint8_t type = r.u8();
				const uint32_t mlen = r.vlq();
				const size_t mStart = r.p;
				if (type == 0x51 && mlen == 3) {
					Tempo x;
					x.tick = tick;
					x.usPerQuarter = double(r.be(3));
					if (x.usPerQuarter > 0) tempos.push_back(x);
				} else if (type == 0x58 && mlen >= 2 && !timeSigSeen) {
					f.timeSigNum = r.u8();
					f.timeSigDen = 1 << r.u8();
					timeSigSeen = true;
				} else if (type == 0x03) {
					std::string name;
					for (uint32_t i = 0; i < mlen; i++) name += char(r.u8());
					f.trackNames.back() = name;
				}
				r.p = mStart + mlen;
				if (type == 0x2F) break;
				continue;
			}
			if (st == 0xF0 || st == 0xF7) {  // sysex
				r.p += r.vlq();
				continue;
			}
			uint8_t d1;
			if (st & 0x80) {
				running = st;
				d1 = r.u8();
			} else {  // running status: st was the first data byte
				d1 = st;
				st = running;
				if (!st) { r.bad = true; break; }
			}
			const int type = st & 0xF0;
			const uint8_t d2 = (type == 0xC0 || type == 0xD0) ? 0 : r.u8();
			Event e;
			e.tick = tick;
			e.track = t;
			e.status = st;
			e.d1 = d1 & 0x7F;
			e.d2 = d2 & 0x7F;
			f.events.push_back(e);
		}
		r.p = trackEnd;
		r.end = data.size();
	}
	if (r.bad) { f.error = "damaged MIDI file"; return f; }

	// tempo map: sorted, one at tick 0, seconds at each change
	std::stable_sort(tempos.begin(), tempos.end(), [](const Tempo& a, const Tempo& b) { return a.tick < b.tick; });
	if (tempos.empty() || tempos[0].tick > 0) tempos.insert(tempos.begin(), Tempo{});
	for (size_t i = 1; i < tempos.size(); i++)
		tempos[i].sec = tempos[i - 1].sec + double(tempos[i].tick - tempos[i - 1].tick) * tempos[i - 1].usPerQuarter * 1e-6 / f.division;
	f.tempos = tempos;

	// merged events: by tick; at the same tick note-offs first (a repeated note ends before it restarts)
	std::stable_sort(f.events.begin(), f.events.end(), [](const Event& a, const Event& b) {
		if (a.tick != b.tick) return a.tick < b.tick;
		return a.isNoteOff() && !b.isNoteOff();
	});
	for (Event& e : f.events) {
		e.sec = f.secAtTick(e.tick);
		f.lengthSec = std::max(f.lengthSec, e.sec);
	}
	f.ok = true;
	return f;
}

} // namespace midifile
