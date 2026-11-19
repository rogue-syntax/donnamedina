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
	void updateAll() {
		for (int l = 0; l < kLanes; l++)
			update(l);
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
};

// Filter (CSEMFilter: setG + doFilter2)
struct alignas(32) Filter4 {
	double q[kLanes], g[kLanes], r[kLanes], a0[kLanes], a[kLanes], rho[kLanes], z1[kLanes], z2[kLanes];

	void gather(int l, const CSEMFilter& f) {
		q[l] = f.qValue(); g[l] = f.g; r[l] = f.R; a0[l] = f.m_dAlpha0; a[l] = f.m_dAlpha; rho[l] = f.m_dRho;
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
	void setGAll(const double* gg) {
		for (int l = 0; l < kLanes; l++)
			setG(l, gg[l]);
	}
	void processAll(float* xn) {
		for (int l = 0; l < kLanes; l++)
			process(l, xn[l]);
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
