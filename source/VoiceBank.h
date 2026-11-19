#pragma once
// Voice bank (2026-11-19, branch simd-voices): the per-sample state of up to kLanes voices, laid out
// "field first": each component (envelope, LFO, oscillator, filter) holds every field as an array
// with one element per voice ("lane"), so one instruction can later work on all lanes at once.
//
// The components mirror the engine's classes: Envelope4 = the envelope part of CLFO, Lfo4 = CLFO's
// LFO, Osc4 = CQBLimitedOscillator (SINE/SAW1/TRI/SQUARE), Filter4 = CSEMFilter, Voice4 = RVoice.
// Each has gather (object -> lane), scatter (lane -> object) and the per-sample step, expression
// for expression the same as the class it mirrors (same types: CLFO float, oscillator and filter
// double), so the result is the same as DXGenerator::singleVoiceProcessing. Keep them in step:
// a change to the voice path must be made in both places (the golden test compares them).
//
// The RVoice objects stay the owners of the state; the bank is a working copy for one block
// (DXGenerator::renderBankGroup: gather, render the block, scatter).
#include "RVoice.h"
#include <cmath>
#include <immintrin.h>

namespace voicebank {

static const int kLanes = 4;

// ---- helpers from the oscillator code ----------------------------------------------------------

// QBLimitedOscillator.cpp
inline double polyBlep(double t, double dt) {
	if (dt <= 0.0) return 0.0;
	if (t < dt) { t /= dt; return t + t - t * t - 1.0; }
	if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
	return 0.0;
}

// CQBLimitedOscillator::doSawtoothO (fo = m_dFo, sr = *m_dSampleRate)
inline double sawO(double dModulo, double dInc, double fo, double sr) {
	double dTrivialSaw = 0.0;
	double dOut = 0.0;
	dTrivialSaw = unipolarToBipolar(dModulo);
	if (fo <= (sr) / 8.0)
		dOut = dTrivialSaw + doBLEP_N(&dBLEPTable_8_BLKHAR[0], 4096, dModulo, fabs(dInc), 1.0, false, 4, false);
	else
		dOut = dTrivialSaw + doBLEP_N(&dBLEPTable[0], 4096, dModulo, fabs(dInc), 1.0, false, 1, false);
	return dOut;
}

// CQBLimitedOscillator::doSquareO (pw = m_dPulseWidth = triCutoff)
inline double squareO(double dModulo, double dInc, double pw, double fo, double sr) {
	double dSaw1 = sawO(dModulo, dInc, fo, sr);
	if (dInc > 0)
		dModulo += pw;
	else
		dModulo -= pw;
	if (dInc > 0 && dModulo >= 1.0)
		dModulo -= 1.0;
	if (dInc < 0 && dModulo <= 0.0)
		dModulo += 1.0;
	double dSaw2 = sawO(dModulo, dInc, fo, sr);
	double dOut = 0.5*dSaw1 - 0.5*dSaw2;
	double dCorr = 1.0 / (pw);
	if ((pw) < 0.5)
		dCorr = 1.0 / (1.0 - (pw));
	dOut *= dCorr;
	return dOut;
}

// CQBLimitedOscillator::doTriangle2
inline double tri2(double dCalcModulo, double triCutoff, double dInc) {
	const bool mirrored = triCutoff > 0.5;
	const double w = mirrored ? 1.0 - triCutoff : triCutoff;
	const double phase = mirrored ? 1.0 - dCalcModulo : dCalcModulo;
	double tempModulo = phase * (w + 0.5);
	double triSample;
	if (tempModulo > 0.5)
		triSample = 1.0 - tempModulo;
	else
		triSample = tempModulo;
	double doOscOut = 2 * (2.0*triSample - 1.0) + 1;
	const double stepHalf = (2.0 - 4.0 * w) * 0.5;
	if (stepHalf > 0.0) {
		const double blep = polyBlep(dCalcModulo, fabs(dInc));
		doOscOut += mirrored ? stepHalf * blep : -stepHalf * blep;
	}
	return doOscOut;
}

// ---- the same, 4 lanes (AVX2) --------------------------------------------------------------------
// Each is the scalar function above with the branches turned into lane masks; the arithmetic
// (operations, order, float or double) is the same, so the results are too.

inline __m256d vabs(__m256d x) { return _mm256_andnot_pd(_mm256_set1_pd(-0.0), x); }
inline __m256d vsel(__m256d mask, __m256d a, __m256d b) { return _mm256_blendv_pd(b, a, mask); }  // mask ? a : b
inline bool vany(__m256d mask) { return _mm256_movemask_pd(mask) != 0; }

// doBLEP_N(table, 4096, d, inc, 1.0, false (falling edge), N, false) with inc = |dInc|.
// The scalar loops over i = 1..N ("d > 1 - i*inc", "d < i*inc") hold for some i exactly when they
// hold for i = N (1 - i*inc decreases with i), so each side is one compare. Index and interpolation
// are float, as in doBLEP_N / dLinTerp (dx = (frac - 0) / (1 - 0) = frac).
inline __m256d blepN(const double* table, __m256d d, __m256d inc, double N) {
	const __m256d zero = _mm256_setzero_pd(), one = _mm256_set1_pd(1.0);
	inc = vsel(_mm256_cmp_pd(inc, zero, _CMP_EQ_OQ), _mm256_set1_pd(0.003), inc);  // dInc == 0 -> 0.003
	const __m256d nInc = _mm256_mul_pd(_mm256_set1_pd(N), inc);
	const __m256d left = _mm256_cmp_pd(d, _mm256_sub_pd(one, nInc), _CMP_GT_OQ);
	const __m256d right = _mm256_andnot_pd(left, _mm256_cmp_pd(d, nInc, _CMP_LT_OQ));
	const __m256d edge = _mm256_or_pd(left, right);
	if (!vany(edge))
		return zero;  // no lane next to the edge (most samples)
	const double center = 4096 / 2.0 - 1;  // dTableCenter
	const __m256d C = _mm256_set1_pd(center);
	// left: fIndex = (1.0 + t) * center, t = (d - 1) / (N*inc); right: t * center + (center + 1), t = d / (N*inc)
	const __m256d fl = _mm256_mul_pd(_mm256_add_pd(one, _mm256_div_pd(_mm256_sub_pd(d, one), nInc)), C);
	const __m256d fr = _mm256_add_pd(_mm256_mul_pd(_mm256_div_pd(d, nInc), C), _mm256_set1_pd(center + 1.0));
	// lanes without an edge read index 0 (result discarded)
	const __m128 fIndex = _mm256_cvtpd_ps(_mm256_and_pd(edge, vsel(left, fl, fr)));
	const __m128i i0 = _mm_cvttps_epi32(fIndex);
	const __m128 frac = _mm_sub_ps(fIndex, _mm_cvtepi32_ps(i0));
	__m128i i1 = _mm_add_epi32(i0, _mm_set1_epi32(1));
	i1 = _mm_and_si128(i1, _mm_cmplt_epi32(i1, _mm_set1_epi32(4096)));  // right side: index + 1 >= len -> table[0]
	const __m128 y1 = _mm256_cvtpd_ps(_mm256_i32gather_pd(table, i0, 8));
	const __m128 y2 = _mm256_cvtpd_ps(_mm256_i32gather_pd(table, i1, 8));
	const __m128 r = _mm_add_ps(_mm_mul_ps(frac, y2), _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), frac), y1));
	// falling edge: dBLEP *= -1.0; height 1.0
	return _mm256_and_pd(edge, _mm256_xor_pd(_mm256_cvtps_pd(r), _mm256_set1_pd(-0.0)));
}

// sawO
inline __m256d sawV(__m256d d, __m256d inc, __m256d fo, __m256d sr) {
	const __m256d triv = _mm256_sub_pd(_mm256_mul_pd(_mm256_set1_pd(2.0), d), _mm256_set1_pd(1.0));  // unipolarToBipolar
	const __m256d absInc = vabs(inc);
	const __m256d m8 = _mm256_cmp_pd(fo, _mm256_div_pd(sr, _mm256_set1_pd(8.0)), _CMP_LE_OQ);  // fo <= sr / 8: 4-point table
	const int bits = _mm256_movemask_pd(m8);
	__m256d blep;
	if (bits == 0xF)
		blep = blepN(&dBLEPTable_8_BLKHAR[0], d, absInc, 4);
	else if (bits == 0)
		blep = blepN(&dBLEPTable[0], d, absInc, 1);
	else
		blep = vsel(m8, blepN(&dBLEPTable_8_BLKHAR[0], d, absInc, 4), blepN(&dBLEPTable[0], d, absInc, 1));
	return _mm256_add_pd(triv, blep);
}

// squareO
inline __m256d squareV(__m256d d, __m256d inc, __m256d pw, __m256d fo, __m256d sr) {
	const __m256d zero = _mm256_setzero_pd(), one = _mm256_set1_pd(1.0), half = _mm256_set1_pd(0.5);
	const __m256d s1 = sawV(d, inc, fo, sr);
	const __m256d pos = _mm256_cmp_pd(inc, zero, _CMP_GT_OQ), neg = _mm256_cmp_pd(inc, zero, _CMP_LT_OQ);
	d = vsel(pos, _mm256_add_pd(d, pw), _mm256_sub_pd(d, pw));
	d = vsel(_mm256_and_pd(pos, _mm256_cmp_pd(d, one, _CMP_GE_OQ)), _mm256_sub_pd(d, one), d);
	d = vsel(_mm256_and_pd(neg, _mm256_cmp_pd(d, zero, _CMP_LE_OQ)), _mm256_add_pd(d, one), d);
	const __m256d s2 = sawV(d, inc, fo, sr);
	const __m256d out = _mm256_sub_pd(_mm256_mul_pd(half, s1), _mm256_mul_pd(half, s2));
	const __m256d corr = vsel(_mm256_cmp_pd(pw, half, _CMP_LT_OQ), _mm256_div_pd(one, _mm256_sub_pd(one, pw)), _mm256_div_pd(one, pw));
	return _mm256_mul_pd(out, corr);
}

// polyBlep
inline __m256d polyBlepV(__m256d t, __m256d dt) {
	const __m256d zero = _mm256_setzero_pd(), one = _mm256_set1_pd(1.0);
	const __m256d valid = _mm256_cmp_pd(dt, zero, _CMP_GT_OQ);  // dt <= 0 -> 0
	const __m256d lo = _mm256_and_pd(valid, _mm256_cmp_pd(t, dt, _CMP_LT_OQ));
	const __m256d hi = _mm256_andnot_pd(lo, _mm256_and_pd(valid, _mm256_cmp_pd(t, _mm256_sub_pd(one, dt), _CMP_GT_OQ)));
	if (!vany(_mm256_or_pd(lo, hi)))
		return zero;
	const __m256d a = _mm256_div_pd(t, dt);                       // t < dt: t /= dt; t + t - t*t - 1
	const __m256d ra = _mm256_sub_pd(_mm256_sub_pd(_mm256_add_pd(a, a), _mm256_mul_pd(a, a)), one);
	const __m256d bq = _mm256_div_pd(_mm256_sub_pd(t, one), dt);  // t > 1 - dt: t = (t - 1)/dt; t*t + t + t + 1
	const __m256d rb = _mm256_add_pd(_mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(bq, bq), bq), bq), one);
	return _mm256_and_pd(_mm256_or_pd(lo, hi), vsel(lo, ra, rb));
}

// tri2
inline __m256d triV(__m256d d, __m256d tri, __m256d inc) {
	const __m256d one = _mm256_set1_pd(1.0), half = _mm256_set1_pd(0.5), two = _mm256_set1_pd(2.0);
	const __m256d mirrored = _mm256_cmp_pd(tri, half, _CMP_GT_OQ);
	const __m256d w = vsel(mirrored, _mm256_sub_pd(one, tri), tri);
	const __m256d phase = vsel(mirrored, _mm256_sub_pd(one, d), d);
	const __m256d tm = _mm256_mul_pd(phase, _mm256_add_pd(w, half));
	const __m256d ts = vsel(_mm256_cmp_pd(tm, half, _CMP_GT_OQ), _mm256_sub_pd(one, tm), tm);
	__m256d out = _mm256_add_pd(_mm256_mul_pd(two, _mm256_sub_pd(_mm256_mul_pd(two, ts), one)), one);  // 2 * (2.0*ts - 1.0) + 1
	const __m256d stepHalf = _mm256_mul_pd(_mm256_sub_pd(two, _mm256_mul_pd(_mm256_set1_pd(4.0), w)), half);
	const __m256d doIt = _mm256_cmp_pd(stepHalf, _mm256_setzero_pd(), _CMP_GT_OQ);
	if (vany(doIt)) {
		const __m256d blep = polyBlepV(d, vabs(inc));
		const __m256d sh = vsel(mirrored, stepHalf, _mm256_xor_pd(stepHalf, _mm256_set1_pd(-0.0)));  // mirrored ? stepHalf : -stepHalf
		out = vsel(doIt, _mm256_add_pd(out, _mm256_mul_pd(sh, blep)), out);
	}
	return out;
}

// ---- components --------------------------------------------------------------------------------

// Modulation sources, as RVoice::envBank: 0 = none (always 0), 1..3 envelopes, 4..6 LFOs. The
// envelopes and LFOs write their output here; destinations read row k for source k.
struct alignas(32) Sources4 {
	float cenv[7][kLanes];
	float& at(int k, int l) { return cenv[k][l]; }
};

// Envelope (envelopes 1..3 are CLFO objects used for doEnvOp's one-pole decay)
struct alignas(32) Envelope4 {
	float env[kLanes], catt[kLanes], cdec[kLanes];

	void gather(int l, const CLFO& f) { env[l] = f.env; catt[l] = f.catt; cdec[l] = f.cdec; }
	void scatter(int l, CLFO& f) const { f.env = env[l]; }
	// singleVoiceProcessing: e = env; env = e * cdec; cenv += catt * (e - cenv)
	void step(int l, float& e, float& cenv) {
		e = env[l];
		env[l] = e * cdec[l];
		cenv += catt[l] * (e - cenv);
	}
	void stepAll(float* e, float* cenv) {
		for (int l = 0; l < kLanes; l++)
			step(l, e[l], cenv[l]);
	}
};

// LFO (CLFO: update() then doOscillate())
struct alignas(32) Lfo4 {
	float mod[kLanes], inc[kLanes], fo[kLanes], foRatio[kLanes], psf[kLanes], foModLin[kLanes];
	float srF[kLanes], delayCounter[kLanes], delayInc[kLanes], ampMod[kLanes], pw[kLanes];
	float interim[kLanes], sawInc[kLanes], chaseStep[kLanes], sawSmooth[kLanes], dOut[kLanes];
	double sr[kLanes];
	bool chase[kLanes];
	unsigned wave[kLanes];

	void gather(int l, const CLFO& f) {
		mod[l] = f.m_dModulo; inc[l] = f.m_dInc; fo[l] = f.m_dFo; foRatio[l] = f.m_dFoRatio;
		psf[l] = f.pitchShiftFactor; foModLin[l] = f.m_dFoModLin; srF[l] = f.m_dSampleRateF;
		delayCounter[l] = f.m_dDelayCounter; delayInc[l] = f.m_dDelayInc; ampMod[l] = f.m_dAmpMod;
		pw[l] = f.m_dPulseWidth; interim[l] = f.interimModulo; sawInc[l] = f.sawModuloInc;
		chaseStep[l] = f.chaseModuloStep; sawSmooth[l] = f.sawSmooth; dOut[l] = f.dOut;
		sr[l] = *f.m_dSampleRate; chase[l] = f.chaseModulo; wave[l] = f.m_uWaveform;
	}
	// (level, rate, phaseStart: what singleVoiceProcessing writes into the LFO every sample)
	void scatter(int l, CLFO& f, float level, float rate, float phaseStart) const {
		f.m_dAmplitude = level; f.m_dOscFo = rate; f.moduloStart = phaseStart;
		f.m_dModulo = mod[l]; f.m_dInc = inc[l]; f.m_dFo = fo[l]; f.m_dDelayCounter = delayCounter[l];
		f.interimModulo = interim[l]; f.sawModuloInc = sawInc[l]; f.chaseModuloStep = chaseStep[l];
		f.chaseModulo = chase[l]; f.dOut = dOut[l]; f.dQPOut = 0.0;
	}

	// CLFOscillator::checkWrapModulo
	static void wrap(float& m_dModulo, float m_dInc) {
		if (m_dInc > 0 && m_dModulo >= 1.0)
			m_dModulo -= 1.0;
		else if (m_dInc < 0 && m_dModulo <= 0.0)
			m_dModulo += 1.0;
	}

	// CLFO::update + doOscillate with m_dAmplitude = level, m_dOscFo = rate; returns the output
	float step(int l, float m_dAmplitude, float m_dOscFo) {
		float& m_dModulo = mod[l];
		float& m_dInc = inc[l];
		float& m_dFo = fo[l];
		float& out = dOut[l];
		// update
		if (delayCounter[l] < 1.0f) {
			delayCounter[l] += delayInc[l];
			m_dFo = 0.0f;
		}
		else {
			m_dFo = m_dOscFo * foRatio[l]*psf[l];
		}
		m_dFo += foModLin[l];
		m_dInc = m_dFo/srF[l];
		// doOscillate
		switch (wave[l]) {
		case 0:  // doSine
			wrap(m_dModulo, m_dInc);
			out = parabolicSine((m_dModulo * 2.0*pi - pi) * -1.0);
			break;
		case 1: {  // doSaw
			float& interimModulo = interim[l];
			float& sawModuloInc = sawInc[l];
			float& chaseModuloStep = chaseStep[l];
			bool& chaseModulo = chase[l];
			if (m_dInc > 0 && m_dModulo >= 1.0) {
				chaseModulo = true;
				m_dModulo -= 1.0;
				interimModulo = .9999;
				sawModuloInc = (1000.0*(interimModulo - m_dModulo)) / sawSmooth[l] / (sr[l]);
				chaseModuloStep = 0;
			}
			if (chaseModulo == true) {
				interimModulo = interimModulo - sawModuloInc;
				out = unipolarToBipolar(interimModulo);
				chaseModuloStep = chaseModuloStep + 1;
				if (chaseModuloStep >= sawSmooth[l]) {
					chaseModulo = false;
				}
			}
			else {
				out = -1 * unipolarToBipolar(m_dModulo);
			}
			break;
		}
		case 2:  // doTri
			wrap(m_dModulo, m_dInc);
			if (m_dModulo < 0.5) {
				out = 2.0*m_dModulo;
			}
			else {
				out = 1.0 - 2.0*(m_dModulo - 0.5);
			}
			out = unipolarToBipolar(out);
			break;
		case 3:  // doSqu
			wrap(m_dModulo, m_dInc);
			out = m_dModulo > pw[l] / 100.0 ? -1.0 : +1.0;
			break;
		}
		m_dModulo += m_dInc;
		return out * m_dAmplitude*ampMod[l];
	}
};

// Oscillator (CQBLimitedOscillator, waveforms SINE/SAW1/TRI/SQUARE)
struct alignas(32) Osc4 {
	double oscFo[kLanes], foRatio[kLanes], psf[kLanes], foModLin[kLanes], fo[kLanes], inc[kLanes];
	double sr[kLanes], tri[kLanes], mod[kLanes], phaseMod[kLanes];
	unsigned wave[kLanes];
	int note[kLanes];

	void gather(int l, const CQBLimitedOscillator& o) {
		oscFo[l] = o.m_dOscFo; foRatio[l] = o.m_dFoRatio; psf[l] = o.pitchShiftFactor;
		foModLin[l] = o.m_dFoModLin; fo[l] = o.m_dFo; inc[l] = o.m_dInc; sr[l] = *o.m_dSampleRate;
		tri[l] = o.triCutoff; mod[l] = o.m_dModulo; phaseMod[l] = o.m_dPhaseMod;
		wave[l] = o.m_uWaveform; note[l] = o.note;
	}
	// (scratch members such as doOscOut, dCalcModulo and m_dPulseWidth are rewritten before they
	// are read, so they are not stored)
	void scatter(int l, CQBLimitedOscillator& o) const {
		o.m_dOscFo = oscFo[l]; o.m_dFoModLin = foModLin[l]; o.m_dFo = fo[l]; o.m_dInc = inc[l];
		o.triCutoff = tri[l]; o.m_dModulo = mod[l]; o.m_dPhaseMod = phaseMod[l];
	}

	// COscillator::update
	void update(int l) {
		fo[l] = oscFo[l] * foRatio[l]*psf[l];
		fo[l] += foModLin[l];
		inc[l] = fo[l]/(sr[l]);
	}
	// update() for all lanes, AVX2 (4 doubles)
	void updateAll() {
		__m256d f = _mm256_mul_pd(_mm256_mul_pd(_mm256_loadu_pd(oscFo), _mm256_loadu_pd(foRatio)), _mm256_loadu_pd(psf));
		f = _mm256_add_pd(f, _mm256_loadu_pd(foModLin));
		_mm256_storeu_pd(fo, f);
		_mm256_storeu_pd(inc, _mm256_div_pd(f, _mm256_loadu_pd(sr)));
	}

	// CQBLimitedOscillator::doOscillate
	double step(int l) {
		const int wrappingBool = (int)(mod[l]);  // checkWrapModulo2
		mod[l] = mod[l] - wrappingBool;
		double dCalcModulo = mod[l] + phaseMod[l];
		checkWrapIndex(dCalcModulo);
		double doOscOut = 0.0;
		switch (wave[l]) {
		case 0: {  // doSine -> SineWave::parabolicSine(-1.0 * dAngle)
			const double dAngle = dCalcModulo * 2.0*(double)pi - (double)pi;
			const double x = -1.0*dAngle;
			doOscOut = SineWave::swB * x + SineWave::swC * x * fabs(x);
			break;
		}
		case 1: doOscOut = sawO(dCalcModulo, inc[l], fo[l], sr[l]); break;               // doSawtooth
		case 2: doOscOut = tri2(dCalcModulo, tri[l], inc[l]); break;                      // doTriangle2
		case 3: doOscOut = squareO(dCalcModulo, inc[l], tri[l], fo[l], sr[l]); break;     // doSquare
		}
		mod[l] += inc[l];  // incModulo
		return doOscOut;
	}

	// step() for all lanes into out[], AVX2 when every lane has the same waveform (the usual case:
	// the waveform comes from the patch; spare lanes copy lane 0), otherwise lane by lane.
	void stepAll(double* out) {
		const unsigned w = wave[0];
		if (w > 3 || wave[1] != w || wave[2] != w || wave[3] != w) {
			for (int l = 0; l < kLanes; l++)
				out[l] = step(l);
			return;
		}
		const __m256d zero = _mm256_setzero_pd(), one = _mm256_set1_pd(1.0);
		__m256d m = _mm256_loadu_pd(mod);
		m = _mm256_sub_pd(m, _mm256_round_pd(m, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC));  // checkWrapModulo2: m -= (int)m
		__m256d dc = _mm256_add_pd(m, _mm256_loadu_pd(phaseMod));
		// checkWrapIndex: while (< 0) += 1; while (>= 1) -= 1 (per lane, the same steps)
		for (__m256d k; vany(k = _mm256_cmp_pd(dc, zero, _CMP_LT_OQ)); )
			dc = vsel(k, _mm256_add_pd(dc, one), dc);
		for (__m256d k; vany(k = _mm256_cmp_pd(dc, one, _CMP_GE_OQ)); )
			dc = vsel(k, _mm256_sub_pd(dc, one), dc);
		const __m256d vinc = _mm256_loadu_pd(inc);
		__m256d r;
		switch (w) {
		case 0: {  // doSine: x = -1.0 * (dc * 2.0 * pi - pi); swB * x + swC * x * fabs(x)
			const __m256d ang = _mm256_sub_pd(_mm256_mul_pd(_mm256_mul_pd(dc, _mm256_set1_pd(2.0)), _mm256_set1_pd((double)pi)), _mm256_set1_pd((double)pi));
			const __m256d x = _mm256_mul_pd(_mm256_set1_pd(-1.0), ang);
			r = _mm256_add_pd(_mm256_mul_pd(_mm256_set1_pd(SineWave::swB), x), _mm256_mul_pd(_mm256_mul_pd(_mm256_set1_pd(SineWave::swC), x), vabs(x)));
			break;
		}
		case 1: r = sawV(dc, vinc, _mm256_loadu_pd(fo), _mm256_loadu_pd(sr)); break;
		case 2: r = triV(dc, _mm256_loadu_pd(tri), vinc); break;
		default: r = squareV(dc, vinc, _mm256_loadu_pd(tri), _mm256_loadu_pd(fo), _mm256_loadu_pd(sr)); break;
		}
		_mm256_storeu_pd(out, r);
		_mm256_storeu_pd(mod, _mm256_add_pd(m, vinc));  // incModulo
	}
};

// Filter (CSEMFilter: setG + doFilter2)
struct alignas(32) Filter4 {
	double q[kLanes], g[kLanes], r[kLanes], a0[kLanes], a[kLanes], rho[kLanes], z1[kLanes], z2[kLanes];

	void gather(int l, const CSEMFilter& f) {
		q[l] = f.qValue(); g[l] = f.g; a0[l] = f.m_dAlpha0; a[l] = f.m_dAlpha; rho[l] = f.m_dRho;
		r[l] = 1.0 / (2.0*q[l]);  // what setG sets every sample (q does not change within a block)
		f.getZ(z1[l], z2[l]);
	}
	void scatter(int l, CSEMFilter& f) const {
		f.g = g[l]; f.R = r[l]; f.m_dAlpha0 = a0[l]; f.m_dAlpha = a[l]; f.m_dRho = rho[l];
		f.setZ(z1[l], z2[l]);
	}
	// CSEMFilter::setG
	void setG(int l, double gg) {
		g[l] = gg;
		r[l] = 1.0 / (2.0*q[l]);
		a0[l] = 1.0 / (1.0 + 2.0*r[l]*g[l] + g[l] * g[l]);
		a[l] = g[l];
		rho[l] = 2.0*r[l] + g[l];
	}
	// CSEMFilter::doFilter2 (lowpass, in place)
	void process(int l, float& xn) {
		double aaa = a0[l] * (xn - rho[l] * z1[l] - z2[l]);
		double bbb = a[l] * aaa + z1[l];
		double ccc = a[l] * bbb + z2[l];
		z1[l] = a[l] * aaa + bbb;
		z2[l] = a[l] * bbb + ccc;
		xn = ccc;
	}
	// setG for all lanes, AVX2. r = 1 / (2q) does not change within a block (q is per voice), so it
	// comes from gather (the same value setG computes every sample).
	void setGAll(__m256d gg) {
		const __m256d two = _mm256_set1_pd(2.0), one = _mm256_set1_pd(1.0);
		const __m256d rr = _mm256_loadu_pd(r);
		_mm256_storeu_pd(g, gg);
		// a0 = 1.0 / (1.0 + 2.0*R*g + g * g)
		_mm256_storeu_pd(a0, _mm256_div_pd(one, _mm256_add_pd(_mm256_add_pd(one, _mm256_mul_pd(_mm256_mul_pd(two, rr), gg)), _mm256_mul_pd(gg, gg))));
		_mm256_storeu_pd(a, gg);
		_mm256_storeu_pd(rho, _mm256_add_pd(_mm256_mul_pd(two, rr), gg));  // 2.0*R + g
	}
	// doFilter2 for all lanes, AVX2 (state in double, signal in and out float)
	void processAll(float* xn) {
		const __m256d x = _mm256_cvtps_pd(_mm_loadu_ps(xn));
		const __m256d A0 = _mm256_loadu_pd(a0), A = _mm256_loadu_pd(a), RHO = _mm256_loadu_pd(rho);
		const __m256d Z1 = _mm256_loadu_pd(z1), Z2 = _mm256_loadu_pd(z2);
		const __m256d aaa = _mm256_mul_pd(A0, _mm256_sub_pd(_mm256_sub_pd(x, _mm256_mul_pd(RHO, Z1)), Z2));
		const __m256d bbb = _mm256_add_pd(_mm256_mul_pd(A, aaa), Z1);
		const __m256d ccc = _mm256_add_pd(_mm256_mul_pd(A, bbb), Z2);
		_mm256_storeu_pd(z1, _mm256_add_pd(_mm256_mul_pd(A, aaa), bbb));
		_mm256_storeu_pd(z2, _mm256_add_pd(_mm256_mul_pd(A, bbb), ccc));
		_mm_storeu_ps(xn, _mm256_cvtpd_ps(ccc));
	}
};

// A group of up to kLanes voices (RVoice)
struct alignas(32) Voice4 {
	static const int kLanes = voicebank::kLanes;
	int count = 0;
	RVoice* voice[kLanes] = {};
	int row[kLanes] = {};  // output row (index in DXGenerator::notes)

	Sources4 src;
	Envelope4 env[4];  // 1..3 (0 unused)
	float e[kLanes];   // RVoice::e (envelope scratch)
	Lfo4 lfo[3];       // voiceEnvLfo[4..6]
	Osc4 carrier;      // WTOsc
	Osc4 modulator;    // WTOsc2
	Filter4 filter;    // m_Filter1

	// RVoice fields the voice path computes
	float osc2out[kLanes], osc1pure[kLanes], gainerTemp[kLanes], o[kLanes], o1[kLanes], o2[kLanes];
	float pan[kLanes], prePan[kLanes], filterEnv[kLanes], tuneC[kLanes], tuneM[kLanes];
	float exprFilter[kLanes], exprFm[kLanes], exprWidth[kLanes];
	float driftPitch[kLanes], driftFilter[kLanes], driftPan[kLanes];
	int newInt[kLanes], newOct[kLanes], newModInt[kLanes], newModOct[kLanes], newNote[kLanes];

	void gather(int l, const RVoice& v) {
		for (int k = 0; k < 7; k++)
			src.cenv[k][l] = v.voiceEnvLfo[k].cenv;
		for (int k = 1; k <= 3; k++)
			env[k].gather(l, v.voiceEnvLfo[k]);
		e[l] = v.e;
		for (int i = 0; i < 3; i++)
			lfo[i].gather(l, v.voiceEnvLfo[4 + i]);
		carrier.gather(l, v.WTOsc);
		modulator.gather(l, v.WTOsc2);
		filter.gather(l, v.m_Filter1);
		osc2out[l] = v.osc2out; osc1pure[l] = v.osc1pure; gainerTemp[l] = v.gainerTemp;
		o[l] = v.o; o1[l] = v.o1; o2[l] = v.o2; pan[l] = v.pan; prePan[l] = v.prePan;
		filterEnv[l] = v.filterEnv; tuneC[l] = v.tuneC; tuneM[l] = v.tuneM;
		driftPitch[l] = v.driftPitch; driftFilter[l] = v.driftFilter; driftPan[l] = v.driftPan;
		newInt[l] = v.newInt; newOct[l] = v.newOct; newModInt[l] = v.newModInt;
		newModOct[l] = v.newModOct; newNote[l] = v.newNote;
	}
	// lfoOn/level/rate/phase: which LFOs ran this block and what was written into them
	void scatter(int l, RVoice& v, const bool lfoOn[3], const float level[3], const float rate[3],
	             const float phase[3]) const {
		v.voiceEnvLfo[0].env = 0.0;
		for (int k = 0; k < 7; k++)
			v.voiceEnvLfo[k].cenv = src.cenv[k][l];
		for (int k = 1; k <= 3; k++)
			env[k].scatter(l, v.voiceEnvLfo[k]);
		v.e = e[l];
		for (int i = 0; i < 3; i++)
			if (lfoOn[i])  // an LFO that did not run is untouched
				lfo[i].scatter(l, v.voiceEnvLfo[4 + i], level[i], rate[i], phase[i]);
		carrier.scatter(l, v.WTOsc);
		modulator.scatter(l, v.WTOsc2);
		filter.scatter(l, v.m_Filter1);
		v.osc2out = osc2out[l]; v.osc1pure = osc1pure[l]; v.gainerTemp = gainerTemp[l];
		v.o = o[l]; v.o1 = o1[l]; v.o2 = o2[l]; v.pan = pan[l]; v.prePan = prePan[l];
		v.filterEnv = filterEnv[l]; v.tuneC = tuneC[l]; v.tuneM = tuneM[l];
		v.newInt = newInt[l]; v.newOct = newOct[l]; v.newModInt = newModInt[l];
		v.newModOct = newModOct[l]; v.newNote = newNote[l];
		v.exprFilter = exprFilter[l]; v.exprFm = exprFm[l]; v.exprWidth = exprWidth[l];
	}
};

}  // namespace voicebank

using VoiceBank = voicebank::Voice4;
