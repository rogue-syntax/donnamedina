// SIMD voice prototype (branch simd-voices, 2026-11-18). Standalone: no plugin code is linked.
//
// The per-sample work of DXGenerator::singleVoiceProcessing for the phase-mod path with sine
// oscillators (envelopes, expression, carrier and modulator pitch with their env offsets and table
// lookups, width cutoff, phase mod, gain, filter coefficient from the g table, SEM filter, pan glide),
// transcribed three ways and timed on the same voices:
//   A  scalar, one voice at a time, one object per voice (the engine's layout today)
//   B  4 voices per AVX2 register, double (same precision as A: output should match to ~1e-12)
//   C  8 voices per AVX2 register, float (twice the lanes; output differs by float rounding)
//   D  as C, but the oscillator phases are 32-bit fixed point (wrap for free, no float phase drift)
// Run: simd_proto.exe [voices] [seconds]      (default 32 voices, 5 s)
#include <immintrin.h>
#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>

namespace {
constexpr double kSR = 44100.0;
constexpr double kPi = 3.14159265358979323846;
constexpr int kTable = 4096;          // filter g table (DXGenerator::kFiltGTableSize)
constexpr double kXMin = 0.01;        // DXGenerator::kFiltXMin
constexpr double kFoMin = 20.0;       // OSC_FO_MIN
constexpr int kMaxVoices = 64;

// parabolicSine (SineWave.cpp): B*x + C*x*|x|
const double swB = 4.0 / (float)kPi, swC = -4.0 / ((float)kPi * (float)kPi);

double midiFreq[128];
float midiFreqF[128];
double gTable[kTable + 1];
float gTableF[kTable + 1];
const double gScale = kTable / (1.0 - kXMin);

// Shared ("global") settings: the knobs and the per-block choices. Values chosen so every term is
// live (non-zero env amounts), not to sound like a preset.
struct Globals {
	double intEnvAmt = 0.2, octEnvAmt = 0.15, tune = 0.01, tuneEnvAmt = 0.02, fine = 0.3, fineEnvAmt = 0.1;
	double modIntEnvAmt = 0.1, modOctEnvAmt = 0.1, modTune = 0.0, modTuneEnvAmt = 0.03, modFine = 0.2, modFineEnvAmt = 0.1;
	int ctrlInt = 0, ctrlOct = 3, ctrlModInt = 7, ctrlModOct = 6;
	double width = 0.5, widthEnv = 0.3, widthPole = 1.0, modWidth = 0.5, modWidthEnv = 0.2, modWidthPole = 1.0;
	double modGain = 0.4, modGainEnv = 0.3, modOscGain = 0.25;
	double gain = 0.7, gainEnv = 0.2, silence = 0.001;
	double filter = 0.55, filterEnv = 0.3, q = 0.9;
	double panLevel = 0.1, panEnvAmt = 0.4, panCoef = 0.0075;
	double exprAmount = 0.3, exprDepthFilter = 0.2, exprDepthFm = 0.3, exprDepthWidth = 0.1;
	bool exprVel = true;
	// modulation sources (envBank index: 1..3 = the voice's envelopes 1..3)
	int srcInt = 2, srcOct = 3, srcTune = 2, srcFine = 3, srcWidth = 2, srcModInt = 3, srcModOct = 2, srcModTune = 3,
	    srcModFine = 2, srcModWidth = 3, srcModGain = 2, srcGain = 3, srcFilter = 2, srcPan = 3;
};
Globals G;

// per-voice start values, shared by all three versions
struct VoiceInit {
	int note1, note2;
	double vel, drift, driftFilter, driftPan;
	double catt[4], cdec[4], env0[4];
	double phase1, phase2;
};
std::vector<VoiceInit> makeVoices(int n) {
	std::vector<VoiceInit> v(n);
	unsigned r = 12345;
	auto rnd = [&] { r = r * 1664525u + 1013904223u; return (r >> 8) * (1.0 / 16777216.0); };
	for (int i = 0; i < n; i++) {
		v[i].note1 = 36 + (i * 7) % 48;
		v[i].note2 = v[i].note1;
		v[i].vel = 60 + 60 * rnd();
		v[i].drift = (float)(1.0 + 0.002 * (rnd() - 0.5));
		v[i].driftFilter = 0.02 * (rnd() - 0.5);
		v[i].driftPan = 0.1 * (rnd() - 0.5);
		for (int e = 0; e < 4; e++) {
			// float-representable, so A/B (double) and C/D (float) decay at the same rate: a decay
			// coefficient rounded to float changes the decay time by ~0.5% (inaudible, but over 5 s
			// it would dominate the sample comparison)
			v[i].catt[e] = (float)(0.001 + 0.01 * rnd());
			v[i].cdec[e] = (float)(0.99990 + 0.00009 * rnd());
			v[i].env0[e] = e ? 1.0 : 0.0;
		}
		v[i].phase1 = rnd();
		v[i].phase2 = rnd();
	}
	return v;
}

// ---------------------------------------------------------------- A: scalar, object per voice
// The engine's voice is one big object (RVoice ~ several KB: oscillators, 7 LFO objects, filter,
// envBank pointers, ...). `pad` keeps consecutive voices that far apart in memory.
struct VoiceA {
	double env[4], cenv[4], catt[4], cdec[4];
	const double* envBank[4];
	int note1, note2;
	double vel, drift, driftFilter, driftPan;
	double exprFilter, exprFm, exprWidth;
	double mod1, inc1, fo1, phaseMod1, foModLin1, triCutoff1;
	double mod2, inc2, fo2, triCutoff2;
	double osc1pure, osc2out, gainerTemp;
	float o, o1, o2;
	double z11, z12, a0, alpha, rho, g, R;
	double filterEnv, pan, prePan;
	char pad[2048];
};

inline double clampd(double x, double lo, double hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline double parabolicSine(double x) { return swB * x + swC * x * std::fabs(x); }
inline double oscA(double& modulo, double inc, double phaseMod) {
	int wrap = (int)modulo;  // checkWrapModulo2
	modulo -= wrap;
	double calc = modulo + phaseMod;
	while (calc < 0.0) calc += 1.0;  // checkWrapIndex
	while (calc >= 1.0) calc -= 1.0;
	const double out = parabolicSine(-1.0 * (calc * 2.0 * kPi - kPi));
	modulo += inc;
	return out;
}

void initA(VoiceA* v, const std::vector<VoiceInit>& in) {
	for (size_t i = 0; i < in.size(); i++) {
		VoiceA& a = v[i];
		std::memset(&a, 0, sizeof(a));
		for (int e = 0; e < 4; e++) { a.env[e] = in[i].env0[e]; a.cenv[e] = 0; a.catt[e] = in[i].catt[e]; a.cdec[e] = in[i].cdec[e]; a.envBank[e] = &a.cenv[e]; }
		a.note1 = in[i].note1; a.note2 = in[i].note2; a.vel = in[i].vel;
		a.drift = in[i].drift; a.driftFilter = in[i].driftFilter; a.driftPan = in[i].driftPan;
		a.mod1 = in[i].phase1; a.mod2 = in[i].phase2;
	}
}

void renderA(VoiceA* v, int nv, int frames, float* L, float* R) {
	for (int s = 0; s < frames; s++) {
		float oL = 0, oR = 0;
		for (int n = 0; n < nv; n++) {
			VoiceA& a = v[n];
			const double e = G.exprVel ? G.exprAmount * (a.vel * (1.0 / 127.0)) : G.exprAmount;
			a.exprFilter = e * G.exprDepthFilter; a.exprFm = e * G.exprDepthFm; a.exprWidth = e * G.exprDepthWidth;
			for (int k = 1; k <= 3; k++) {
				const double ee = a.env[k];
				a.env[k] = ee * a.cdec[k];
				a.cenv[k] += a.catt[k] * (ee - a.cenv[k]);
			}
			const double* const* bank = a.envBank;
			// doPhaseMod: the modulator runs first, with last sample's pitch (as in the engine)
			a.inc2 = a.fo2 / kSR;
			a.osc2out = oscA(a.mod2, a.inc2, 0.0);
			a.phaseMod1 = clampd(G.modGain + a.exprFm + G.modGainEnv * *bank[G.srcModGain], 0.003, 1.0) * a.osc2out;
			a.foModLin1 = 0.0;
			// carrier pitch
			const int newInt = G.ctrlInt + (int)(G.intEnvAmt * *bank[G.srcInt] * 13);
			int newOct = G.ctrlOct + (int)(G.octEnvAmt * *bank[G.srcOct] * 6);
			newOct = newOct < 1 ? 1 : (newOct > 5 ? 5 : newOct);
			int note = (int)(a.note1 + (newInt + (newOct - 3) * 12.0));
			note = note < 0 ? 0 : (note > 127 ? 127 : note);
			const double tuneC = (G.tune + G.tuneEnvAmt * *bank[G.srcTune]) + (G.fine + G.fineEnvAmt * *bank[G.srcFine]) / 100;
			a.fo1 = std::fmax(kFoMin, (midiFreq[note] + midiFreq[note] * 2 * tuneC) * a.drift);
			a.inc1 = (a.fo1 + a.foModLin1) / kSR;  // update()
			a.triCutoff1 = clampd(G.width + a.exprWidth + G.widthEnv * G.widthPole * *bank[G.srcWidth], 0.1, 0.9);
			// modulator pitch
			const int newModInt = G.ctrlModInt + (int)(G.modIntEnvAmt * *bank[G.srcModInt] * 13);
			const int newModOct = G.ctrlModOct + (int)(G.modOctEnvAmt * *bank[G.srcModOct] * 13);
			int mnote = (int)(a.note2 + (newModInt + (newModOct - 6) * 12.0));
			mnote = mnote < 1 ? 1 : (mnote > 109 ? 109 : mnote);
			const double tuneM = (G.modTune + G.modTuneEnvAmt * *bank[G.srcModTune]) + (G.modFine + G.modFineEnvAmt * *bank[G.srcModFine]) / 100;
			a.fo2 = std::fmax(kFoMin, (midiFreq[mnote] + midiFreq[mnote] * 2 * tuneM) * a.drift);
			a.triCutoff2 = clampd(G.modWidth + a.exprWidth + G.modWidthEnv * G.modWidthPole * *bank[G.srcModWidth], 0.1, 0.9);
			// carrier
			a.osc1pure = oscA(a.mod1, a.inc1, a.phaseMod1);
			a.gainerTemp = clampd(G.gain + G.gainEnv * *bank[G.srcGain], G.silence, 1.0);
			a.o = (float)(a.gainerTemp * (a.cenv[1] * (a.osc1pure + a.osc2out * G.modOscGain)));
			// filter
			a.filterEnv = G.filterEnv * *bank[G.srcFilter];
			const double fx = clampd(G.filter + a.filterEnv + a.exprFilter + a.driftFilter, 0.01, 1.0);
			const double pos = (fx - kXMin) * gScale;
			int idx = (int)pos;
			if (idx >= kTable) idx = kTable - 1;
			const double frac = pos - idx;
			a.g = gTable[idx] + (gTable[idx + 1] - gTable[idx]) * frac;
			a.R = 1.0 / (2.0 * G.q);
			a.a0 = 1.0 / (1.0 + 2.0 * a.R * a.g + a.g * a.g);
			a.alpha = a.g;
			a.rho = 2.0 * a.R + a.g;
			const double aaa = a.a0 * (a.o - a.rho * a.z11 - a.z12);
			const double bbb = a.alpha * aaa + a.z11;
			const double ccc = a.alpha * bbb + a.z12;
			a.z11 = a.alpha * aaa + bbb;
			a.z12 = a.alpha * bbb + ccc;
			a.o = (float)ccc;
			// pan
			a.pan = clampd(G.panLevel + a.driftPan + G.panEnvAmt * *bank[G.srcPan], -1, 1);
			a.prePan += G.panCoef * (a.pan - a.prePan);
			a.o1 = (float)(((a.prePan + 1.0) / 2.0) * a.o);
			a.o2 = (float)(((a.prePan - 1.0) / -2.0) * a.o);
			oL += a.o1; oR += a.o2;
		}
		L[s] = oL; R[s] = oR;
	}
}

// ---------------------------------------------------------------- B / C: lanes of voices
// A thin vector type so one kernel serves both widths.
struct D4 {
	__m256d v;
	using T = double;
	static constexpr int N = 4;
	D4() = default;
	D4(__m256d x) : v(x) {}
	D4(double x) : v(_mm256_set1_pd(x)) {}
	static D4 load(const double* p) { return _mm256_load_pd(p); }
	void store(double* p) const { _mm256_store_pd(p, v); }
	friend D4 operator+(D4 a, D4 b) { return _mm256_add_pd(a.v, b.v); }
	friend D4 operator-(D4 a, D4 b) { return _mm256_sub_pd(a.v, b.v); }
	friend D4 operator*(D4 a, D4 b) { return _mm256_mul_pd(a.v, b.v); }
	friend D4 operator/(D4 a, D4 b) { return _mm256_div_pd(a.v, b.v); }
	friend D4 vmin(D4 a, D4 b) { return _mm256_min_pd(a.v, b.v); }
	friend D4 vmax(D4 a, D4 b) { return _mm256_max_pd(a.v, b.v); }
	friend D4 vtrunc(D4 a) { return _mm256_round_pd(a.v, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC); }
	friend D4 vfloor(D4 a) { return _mm256_floor_pd(a.v); }
	friend D4 vabs(D4 a) { return _mm256_andnot_pd(_mm256_set1_pd(-0.0), a.v); }
	// table[(int)i] for integral-valued i
	friend D4 gather(const double* table, D4 i) { return _mm256_i32gather_pd(table, _mm256_cvttpd_epi32(i.v), 8); }
	friend D4 gather1(const double* table, D4 i) { return _mm256_i32gather_pd(table + 1, _mm256_cvttpd_epi32(i.v), 8); }
};
struct F8 {
	__m256 v;
	using T = float;
	static constexpr int N = 8;
	F8() = default;
	F8(__m256 x) : v(x) {}
	F8(double x) : v(_mm256_set1_ps((float)x)) {}
	static F8 load(const float* p) { return _mm256_load_ps(p); }
	void store(float* p) const { _mm256_store_ps(p, v); }
	friend F8 operator+(F8 a, F8 b) { return _mm256_add_ps(a.v, b.v); }
	friend F8 operator-(F8 a, F8 b) { return _mm256_sub_ps(a.v, b.v); }
	friend F8 operator*(F8 a, F8 b) { return _mm256_mul_ps(a.v, b.v); }
	friend F8 operator/(F8 a, F8 b) { return _mm256_div_ps(a.v, b.v); }
	friend F8 vmin(F8 a, F8 b) { return _mm256_min_ps(a.v, b.v); }
	friend F8 vmax(F8 a, F8 b) { return _mm256_max_ps(a.v, b.v); }
	friend F8 vtrunc(F8 a) { return _mm256_round_ps(a.v, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC); }
	friend F8 vfloor(F8 a) { return _mm256_floor_ps(a.v); }
	friend F8 vabs(F8 a) { return _mm256_andnot_ps(_mm256_set1_ps(-0.0f), a.v); }
	friend F8 gather(const float* table, F8 i) { return _mm256_i32gather_ps(table, _mm256_cvttps_epi32(i.v), 4); }
	friend F8 gather1(const float* table, F8 i) { return _mm256_i32gather_ps(table + 1, _mm256_cvttps_epi32(i.v), 4); }
};
template <class V> V vclamp(V x, double lo, double hi) { return vmin(vmax(x, V(lo)), V(hi)); }

// Structure of arrays: each field is an array over voices, so lanes load side by side.
template <class T>
struct Bank {
	alignas(32) T env[4][kMaxVoices], cenv[4][kMaxVoices], catt[4][kMaxVoices], cdec[4][kMaxVoices];
	alignas(32) T note1[kMaxVoices], note2[kMaxVoices], vel[kMaxVoices], drift[kMaxVoices], driftFilter[kMaxVoices], driftPan[kMaxVoices];
	alignas(32) T mod1[kMaxVoices], mod2[kMaxVoices], fo2[kMaxVoices];
	alignas(32) T z11[kMaxVoices], z12[kMaxVoices], prePan[kMaxVoices];
	alignas(32) T tri1[kMaxVoices], tri2[kMaxVoices];  // width cutoffs (computed as the engine does; unused by sine)
	alignas(32) uint32_t ph1[kMaxVoices], ph2[kMaxVoices];  // D: fixed-point phases, 1.0 = 2^32
	int count = 0;  // active voices, packed at the front; the rest of the last vector are silent lanes
};

template <class T>
void initBank(Bank<T>& b, const std::vector<VoiceInit>& in) {
	std::memset(&b, 0, sizeof(b));
	b.count = (int)in.size();
	for (size_t i = 0; i < in.size(); i++) {
		for (int e = 0; e < 4; e++) { b.env[e][i] = (T)in[i].env0[e]; b.catt[e][i] = (T)in[i].catt[e]; b.cdec[e][i] = (T)in[i].cdec[e]; }
		b.note1[i] = (T)in[i].note1; b.note2[i] = (T)in[i].note2; b.vel[i] = (T)in[i].vel;
		b.drift[i] = (T)in[i].drift; b.driftFilter[i] = (T)in[i].driftFilter; b.driftPan[i] = (T)in[i].driftPan;
		b.mod1[i] = (T)in[i].phase1; b.mod2[i] = (T)in[i].phase2;
		b.ph1[i] = (uint32_t)(in[i].phase1 * 4294967296.0); b.ph2[i] = (uint32_t)(in[i].phase2 * 4294967296.0);
	}
}

// D: fixed-point phase. The phase is an unsigned 32-bit count (1 cycle = 2^32), so the wrap is the
// integer overflow; inc = fo / fs * 2^32. Read out as float in [0, 1) from the top 24 bits.
inline F8 oscFixed(uint32_t* ph, F8 inc, F8 phaseMod) {
	const __m256i p = _mm256_load_si256(reinterpret_cast<const __m256i*>(ph));
	const F8 modulo = _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_srli_epi32(p, 8)), _mm256_set1_ps(1.0f / 16777216.0f));
	F8 calc = modulo + phaseMod;
	calc = calc - vfloor(calc);
	const F8 x = F8(-1.0) * (calc * F8(2.0 * kPi) - F8(kPi));
	const F8 out = F8(swB) * x + F8(swC) * x * vabs(x);
	// inc < 0.5 cycle (fo < fs/2), so it fits a signed 32-bit conversion of inc * 2^32 / 2, doubled
	const __m256i di = _mm256_slli_epi32(_mm256_cvttps_epi32(_mm256_mul_ps(inc.v, _mm256_set1_ps(2147483648.0f))), 1);
	_mm256_store_si256(reinterpret_cast<__m256i*>(ph), _mm256_add_epi32(p, di));
	return out;
}

template <class V>
inline V oscV(V& modulo, V inc, V phaseMod) {
	modulo = modulo - vtrunc(modulo);
	V calc = modulo + phaseMod;
	calc = calc - vfloor(calc);  // == the while loops of checkWrapIndex
	const V x = V(-1.0) * (calc * V(2.0 * kPi) - V(kPi));
	const V out = V(swB) * x + V(swC) * x * vabs(x);
	modulo = modulo + inc;
	return out;
}

template <class V, bool kFixed = false>
void renderV(Bank<typename V::T>& b, int frames, float* L, float* R, const typename V::T* freq, const typename V::T* gt) {
	using T = typename V::T;
	const int nvec = (b.count + V::N - 1) / V::N;
	// per block: the source choices become array pointers (no per-voice pointer chase)
	auto src = [&](int k) { return b.cenv[k]; };
	const T *sInt = src(G.srcInt), *sOct = src(G.srcOct), *sTune = src(G.srcTune), *sFine = src(G.srcFine), *sWidth = src(G.srcWidth),
	        *sModInt = src(G.srcModInt), *sModOct = src(G.srcModOct), *sModTune = src(G.srcModTune), *sModFine = src(G.srcModFine),
	        *sModWidth = src(G.srcModWidth), *sModGain = src(G.srcModGain), *sGain = src(G.srcGain), *sFilter = src(G.srcFilter), *sPan = src(G.srcPan);
	const double Rq = 1.0 / (2.0 * G.q);
	alignas(32) T lanesL[V::N], lanesR[V::N];
	for (int s = 0; s < frames; s++) {
		V accL(0.0), accR(0.0);
		for (int k = 0; k < nvec; k++) {
			const int o = k * V::N;
			// silent lanes past b.count: their envelopes are 0 (env0 = 0), so they add nothing
			const V e = G.exprVel ? V(G.exprAmount) * (V::load(b.vel + o) * V(1.0 / 127.0)) : V(G.exprAmount);
			const V exprFilter = e * V(G.exprDepthFilter), exprFm = e * V(G.exprDepthFm), exprWidth = e * V(G.exprDepthWidth);
			for (int j = 1; j <= 3; j++) {
				const V ee = V::load(b.env[j] + o);
				(ee * V::load(b.cdec[j] + o)).store(b.env[j] + o);
				V c = V::load(b.cenv[j] + o);
				c = c + V::load(b.catt[j] + o) * (ee - c);
				c.store(b.cenv[j] + o);
			}
			const V drift = V::load(b.drift + o);
			// modulator (last sample's pitch, as in the engine)
			V fo2 = V::load(b.fo2 + o);
			V osc2;
			if constexpr (kFixed) osc2 = oscFixed(b.ph2 + o, fo2 / V(kSR), V(0.0));
			else {
				V mod2 = V::load(b.mod2 + o);
				osc2 = oscV(mod2, fo2 / V(kSR), V(0.0));
				mod2.store(b.mod2 + o);
			}
			const V phaseMod1 = vclamp(V(G.modGain) + exprFm + V(G.modGainEnv) * V::load(sModGain + o), 0.003, 1.0) * osc2;
			// carrier pitch (integral values kept in floating point, truncated like the casts)
			const V newInt = V((double)G.ctrlInt) + vtrunc(V(G.intEnvAmt) * V::load(sInt + o) * V(13.0));
			const V newOct = vclamp(V((double)G.ctrlOct) + vtrunc(V(G.octEnvAmt) * V::load(sOct + o) * V(6.0)), 1.0, 5.0);
			const V note = vclamp(vtrunc(V::load(b.note1 + o) + (newInt + (newOct - V(3.0)) * V(12.0))), 0.0, 127.0);
			const V tuneC = (V(G.tune) + V(G.tuneEnvAmt) * V::load(sTune + o)) + (V(G.fine) + V(G.fineEnvAmt) * V::load(sFine + o)) / V(100.0);
			const V mf = gather(freq, note);
			const V fo1 = vmax(V(kFoMin), (mf + mf * V(2.0) * tuneC) * drift);
			const V inc1 = fo1 / V(kSR);
			vclamp(V(G.width) + exprWidth + V(G.widthEnv * G.widthPole) * V::load(sWidth + o), 0.1, 0.9).store(b.tri1 + o);
			// modulator pitch for the next sample
			const V newModInt = V((double)G.ctrlModInt) + vtrunc(V(G.modIntEnvAmt) * V::load(sModInt + o) * V(13.0));
			const V newModOct = V((double)G.ctrlModOct) + vtrunc(V(G.modOctEnvAmt) * V::load(sModOct + o) * V(13.0));
			const V mnote = vclamp(vtrunc(V::load(b.note2 + o) + (newModInt + (newModOct - V(6.0)) * V(12.0))), 1.0, 109.0);
			const V tuneM = (V(G.modTune) + V(G.modTuneEnvAmt) * V::load(sModTune + o)) + (V(G.modFine) + V(G.modFineEnvAmt) * V::load(sModFine + o)) / V(100.0);
			const V mf2 = gather(freq, mnote);
			vmax(V(kFoMin), (mf2 + mf2 * V(2.0) * tuneM) * drift).store(b.fo2 + o);
			vclamp(V(G.modWidth) + exprWidth + V(G.modWidthEnv * G.modWidthPole) * V::load(sModWidth + o), 0.1, 0.9).store(b.tri2 + o);
			// carrier
			V osc1;
			if constexpr (kFixed) osc1 = oscFixed(b.ph1 + o, inc1, phaseMod1);
			else {
				V mod1 = V::load(b.mod1 + o);
				osc1 = oscV(mod1, inc1, phaseMod1);
				mod1.store(b.mod1 + o);
			}
			const V gain = vclamp(V(G.gain) + V(G.gainEnv) * V::load(sGain + o), G.silence, 1.0);
			V x = gain * (V::load(b.cenv[1] + o) * (osc1 + osc2 * V(G.modOscGain)));
			// filter coefficient: table + lerp (two gathers)
			const V fx = vclamp(V(G.filter) + V(G.filterEnv) * V::load(sFilter + o) + exprFilter + V::load(b.driftFilter + o), 0.01, 1.0);
			const V pos = (fx - V(kXMin)) * V(gScale);
			const V idx = vmin(vtrunc(pos), V(kTable - 1.0));
			const V frac = pos - idx;
			const V g0 = gather(gt, idx), g1 = gather1(gt, idx);
			const V g = g0 + (g1 - g0) * frac;
			const V a0 = V(1.0) / (V(1.0) + V(2.0 * Rq) * g + g * g);
			const V rho = V(2.0 * Rq) + g;
			V z11 = V::load(b.z11 + o), z12 = V::load(b.z12 + o);
			const V aaa = a0 * (x - rho * z11 - z12);
			const V bbb = g * aaa + z11;
			const V ccc = g * bbb + z12;
			(g * aaa + bbb).store(b.z11 + o);
			(g * bbb + ccc).store(b.z12 + o);
			x = ccc;
			// pan
			const V pan = vclamp(V(G.panLevel) + V::load(b.driftPan + o) + V(G.panEnvAmt) * V::load(sPan + o), -1.0, 1.0);
			V pp = V::load(b.prePan + o);
			pp = pp + V(G.panCoef) * (pan - pp);
			pp.store(b.prePan + o);
			accL = accL + ((pp + V(1.0)) * V(0.5)) * x;
			accR = accR + ((V(1.0) - pp) * V(0.5)) * x;
		}
		accL.store(lanesL);
		accR.store(lanesR);
		float l = 0, r = 0;
		for (int i = 0; i < V::N; i++) { l += (float)lanesL[i]; r += (float)lanesR[i]; }
		L[s] = l; R[s] = r;
	}
}

double now() { LARGE_INTEGER c, f; QueryPerformanceCounter(&c); QueryPerformanceFrequency(&f); return (double)c.QuadPart / f.QuadPart; }

void compare(const char* name, const std::vector<float>& ref, const std::vector<float>& x) {
	double sig = 0, err = 0, maxd = 0;
	for (size_t i = 0; i < ref.size(); i++) {
		sig += (double)ref[i] * ref[i];
		const double d = (double)x[i] - ref[i];
		err += d * d;
		maxd = std::fmax(maxd, std::fabs(d));
	}
	std::printf("    %s vs A: difference %.1f dB below the signal, max sample diff %.2e (peak level %.2f)\n", name,
	            err > 0 ? -10.0 * std::log10(err / sig) : 400.0, maxd, [&] { float m = 0; for (float v : ref) m = std::fmax(m, std::fabs(v)); return m; }());
}
}  // namespace

int main(int argc, char** argv) {
	const int nv = argc > 1 ? std::atoi(argv[1]) : 32;
	const double seconds = argc > 2 ? std::atof(argv[2]) : 5.0;
	if (nv < 1 || nv > kMaxVoices) { std::printf("voices 1..%d\n", kMaxVoices); return 1; }
	// "smooth": no env on interval/octave. Those are truncated to whole semitones, and float and double
	// cross an integer on different samples, so the voices jump a semitone at different times; with
	// them off, the comparison shows the float error itself.
	if (argc > 3 && std::strcmp(argv[3], "smooth") == 0) G.intEnvAmt = G.octEnvAmt = G.modIntEnvAmt = G.modOctEnvAmt = 0.0;
	for (int i = 0; i < 128; i++) { midiFreq[i] = 440.0 * std::pow(2.0, (i - 69) / 12.0); midiFreqF[i] = (float)midiFreq[i]; }
	for (int i = 0; i <= kTable; i++) {  // g(x) = tan(pi * fc / fs), fc on a 20 Hz..20 kHz log scale
		const double x = kXMin + (1.0 - kXMin) * i / kTable;
		gTable[i] = std::tan(kPi * 20.0 * std::pow(1000.0, x) / kSR * 0.999);
		gTableF[i] = (float)gTable[i];
	}
	const int frames = (int)(seconds * kSR);
	const auto init = makeVoices(nv);
	std::vector<float> aL(frames), aR(frames), bL(frames), bR(frames), cL(frames), cR(frames), dL(frames), dR(frames);

	auto* va = static_cast<VoiceA*>(_aligned_malloc(sizeof(VoiceA) * kMaxVoices, 64));
	auto* bd = static_cast<Bank<double>*>(_aligned_malloc(sizeof(Bank<double>), 64));
	auto* bf = static_cast<Bank<float>*>(_aligned_malloc(sizeof(Bank<float>), 64));
	auto* bx = static_cast<Bank<float>*>(_aligned_malloc(sizeof(Bank<float>), 64));
	double best[4] = {1e9, 1e9, 1e9, 1e9};
	for (int rep = 0; rep < 3; rep++) {  // best of 3
		initA(va, init); double t = now(); renderA(va, nv, frames, aL.data(), aR.data()); best[0] = std::fmin(best[0], now() - t);
		initBank(*bd, init); t = now(); renderV<D4>(*bd, frames, bL.data(), bR.data(), midiFreq, gTable); best[1] = std::fmin(best[1], now() - t);
		initBank(*bf, init); t = now(); renderV<F8>(*bf, frames, cL.data(), cR.data(), midiFreqF, gTableF); best[2] = std::fmin(best[2], now() - t);
		initBank(*bx, init); t = now(); renderV<F8, true>(*bx, frames, dL.data(), dR.data(), midiFreqF, gTableF); best[3] = std::fmin(best[3], now() - t);
	}
	std::printf("simd_proto: %d voices, %.0f s at 44.1 kHz (phase-mod sine path of singleVoiceProcessing), best of 3\n", nv, seconds);
	const char* names[4] = {"A scalar, object per voice  ", "B 4 lanes double (AVX2)     ", "C 8 lanes float (AVX2)      ", "D 8 lanes float, fixed phase"};
	for (int i = 0; i < 4; i++)
		std::printf("  %s %.3f s = %5.2f%% of one core, %5.1f ns per voice-sample, %.2fx\n", names[i], best[i], 100.0 * best[i] / seconds,
		            best[i] * 1e9 / ((double)frames * nv), best[0] / best[i]);
	std::vector<float> a(aL), b(bL), c(cL), d(dL);
	a.insert(a.end(), aR.begin(), aR.end()); b.insert(b.end(), bR.begin(), bR.end()); c.insert(c.end(), cR.begin(), cR.end()); d.insert(d.end(), dR.begin(), dR.end());
	compare("B", a, b);
	compare("C", a, c);
	compare("D", a, d);
	_aligned_free(va); _aligned_free(bd); _aligned_free(bf); _aligned_free(bx);
	return 0;
}
