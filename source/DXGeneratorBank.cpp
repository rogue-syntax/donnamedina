// Voice bank kernel (2026-11-19, branch simd-voices, step 2; see VoiceBank.h).
//
// renderBankGroup renders one block for up to VoiceBank::kLanes voices: gather (RVoice -> bank),
// the sample loop with the lane loop inside it, scatter (bank -> RVoice). The per-lane code is
// DXGenerator::singleVoiceProcessing and what it calls (CLFO::update/doOscillate,
// CQBLimitedOscillator::update/doOscillate for SINE/SAW1/TRI/SQUARE, the doXxxMod functions,
// CSEMFilter::setG/doFilter2), expression for expression, with the envBank pointers resolved to
// bank.cenv indices and the member-function dispatch replaced by switches. Keep the two in step:
// a change to the voice path must be made in both (the golden test compares them: run it with the
// bank on and off, see note_off_test "golden").
//
// Step 3 turns the lane loop into SIMD (the same arithmetic per lane, several lanes per instruction).
#include "mdaDX10Processor.h"
#include "DXGenerator.h"
#include <cmath>

namespace Steinberg {
namespace Vst {
namespace rs {

namespace {

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

// CQBLimitedOscillator::doOscillate for waveforms 0..3 (bankEligible)
inline double oscStep(double& mod, double inc, double phaseMod, unsigned wave, double fo, double sr, double tri) {
	const int wrappingBool = (int)(mod);  // COscillator::checkWrapModulo2
	mod = mod - wrappingBool;
	double dCalcModulo = mod + phaseMod;
	checkWrapIndex(dCalcModulo);
	double doOscOut = 0.0;
	switch (wave) {
	case 0: {  // doSine -> SineWave::parabolicSine(-1.0 * dAngle)
		const double dAngle = dCalcModulo * 2.0*(double)pi - (double)pi;
		const double x = -1.0*dAngle;
		doOscOut = SineWave::swB * x + SineWave::swC * x * fabs(x);
		break;
	}
	case 1: doOscOut = sawO(dCalcModulo, inc, fo, sr); break;
	case 2: doOscOut = tri2(dCalcModulo, tri, inc); break;
	case 3: doOscOut = squareO(dCalcModulo, inc, tri, fo, sr); break;
	}
	mod += inc;  // incModulo
	return doOscOut;
}

// COscillator::update
inline void oscUpdate(VoiceBank& b, int j, int l) {
	b.oFo[j][l] = b.oOscFo[j][l] * b.oFoRatio[j][l]*b.oPsf[j][l];
	b.oFo[j][l] += b.oFoModLin[j][l];
	b.oInc[j][l] = b.oFo[j][l]/(b.oSr[j][l]);
}

// CLFOscillator::checkWrapModulo
inline void lfoWrap(float& m_dModulo, float m_dInc) {
	if (m_dInc > 0 && m_dModulo >= 1.0)
		m_dModulo -= 1.0;
	else if (m_dInc < 0 && m_dModulo <= 0.0)
		m_dModulo += 1.0;
}

// CLFO: update() then doOscillate(), with m_dAmplitude = level and m_dOscFo = rate
inline float lfoStep(VoiceBank& b, int i, int l, float m_dAmplitude, float m_dOscFo) {
	float& m_dModulo = b.lMod[i][l];
	float& m_dInc = b.lInc[i][l];
	float& m_dFo = b.lFo[i][l];
	float& dOut = b.lDOut[i][l];
	if (b.lDelayCounter[i][l] < 1.0f) {
		b.lDelayCounter[i][l] += b.lDelayInc[i][l];
		m_dFo = 0.0f;
	}
	else {
		m_dFo = m_dOscFo * b.lFoRatio[i][l]*b.lPsf[i][l];
	}
	m_dFo += b.lFoModLin[i][l];
	m_dInc = m_dFo/b.lSrF[i][l];

	switch (b.lWave[i][l]) {
	case 0:
		lfoWrap(m_dModulo, m_dInc);
		dOut = parabolicSine((m_dModulo * 2.0*pi - pi) * -1.0);
		break;
	case 1: {  // CLFO::doSaw
		float& interimModulo = b.lInterim[i][l];
		float& sawModuloInc = b.lSawInc[i][l];
		float& chaseModuloStep = b.lChaseStep[i][l];
		bool& chaseModulo = b.lChase[i][l];
		if (m_dInc > 0 && m_dModulo >= 1.0) {
			chaseModulo = true;
			m_dModulo -= 1.0;
			interimModulo = .9999;
			sawModuloInc = (1000.0*(interimModulo - m_dModulo)) / b.lSawSmooth[i][l] / (b.lSr[i][l]);
			chaseModuloStep = 0;
		}
		if (chaseModulo == true) {
			interimModulo = interimModulo - sawModuloInc;
			dOut = unipolarToBipolar(interimModulo);
			chaseModuloStep = chaseModuloStep + 1;
			if (chaseModuloStep >= b.lSawSmooth[i][l]) {
				chaseModulo = false;
			}
		}
		else {
			dOut = -1 * unipolarToBipolar(m_dModulo);
		}
		break;
	}
	case 2:  // CLFO::doTri
		lfoWrap(m_dModulo, m_dInc);
		if (m_dModulo < 0.5) {
			dOut = 2.0*m_dModulo;
		}
		else {
			dOut = 1.0 - 2.0*(m_dModulo - 0.5);
		}
		dOut = unipolarToBipolar(dOut);
		break;
	case 3:  // CLFO::doSqu
		lfoWrap(m_dModulo, m_dInc);
		dOut = m_dModulo > b.lPw[i][l] / 100.0 ? -1.0 : +1.0;
		break;
	}
	m_dModulo += m_dInc;
	return dOut * m_dAmplitude*b.lAmpMod[i][l];
}

}  // namespace

bool DXGenerator::bankEligible(const RVoice& v) const {
	if (v.rampingDown)
		return false;  // fades (and the restart when they end) stay on the scalar path
	if (v.WTOsc.m_uWaveform > 3 || v.WTOsc2.m_uWaveform > 3)
		return false;  // SINE, SAW1, TRI, SQUARE only
	const int mt = ctrlValues[MOD_TYPE_ + paramStartIdx];
	if (mt < 0 || mt > 3)
		return false;
	const int srcs[] = {
		newIntEnv, newOctEnv, tuneCEnv, fineCEnv, triEnvSrc,
		newModIntEnv, newModOctEnv, modTuneCEnv, modFineCEnv, modTriEnvSrc,
		filterSrc, panEnvSrc,
		ctrlValues[GAIN_ENV_SRC + paramStartIdx], ctrlValues[MOD_GAIN_ENV_SRC + paramStartIdx],
		ctrlValues[MOD_BEAT_ENV_SRC + paramStartIdx],
	};
	for (int s : srcs)
		if (s < 0 || s > 6)
			return false;
	return true;
}

void DXGenerator::renderBankGroup(int n) {
	VoiceBank& b = bank;
	const int L = b.count;
	bankVoiceSamples += static_cast<unsigned long long>(L) * n;

	// ---- gather
	for (int l = 0; l < L; l++) {
		RVoice& v = *b.voice[l];
		for (int k = 0; k < 7; k++)
			b.cenv[k][l] = v.voiceEnvLfo[k].cenv;
		for (int k = 1; k <= 3; k++) {
			b.env[k][l] = v.voiceEnvLfo[k].env;
			b.catt[k][l] = v.voiceEnvLfo[k].catt;
			b.cdec[k][l] = v.voiceEnvLfo[k].cdec;
		}
		b.e[l] = v.e;
		for (int i = 0; i < 3; i++) {
			const CLFO& f = v.voiceEnvLfo[4 + i];
			b.lMod[i][l] = f.m_dModulo;
			b.lInc[i][l] = f.m_dInc;
			b.lFo[i][l] = f.m_dFo;
			b.lFoRatio[i][l] = f.m_dFoRatio;
			b.lPsf[i][l] = f.pitchShiftFactor;
			b.lFoModLin[i][l] = f.m_dFoModLin;
			b.lSrF[i][l] = f.m_dSampleRateF;
			b.lDelayCounter[i][l] = f.m_dDelayCounter;
			b.lDelayInc[i][l] = f.m_dDelayInc;
			b.lAmpMod[i][l] = f.m_dAmpMod;
			b.lPw[i][l] = f.m_dPulseWidth;
			b.lInterim[i][l] = f.interimModulo;
			b.lSawInc[i][l] = f.sawModuloInc;
			b.lChaseStep[i][l] = f.chaseModuloStep;
			b.lSawSmooth[i][l] = f.sawSmooth;
			b.lDOut[i][l] = f.dOut;
			b.lSr[i][l] = *f.m_dSampleRate;
			b.lChase[i][l] = f.chaseModulo;
			b.lWave[i][l] = f.m_uWaveform;
		}
		const CQBLimitedOscillator* osc[2] = {&v.WTOsc, &v.WTOsc2};
		for (int j = 0; j < 2; j++) {
			b.oOscFo[j][l] = osc[j]->m_dOscFo;
			b.oFoRatio[j][l] = osc[j]->m_dFoRatio;
			b.oPsf[j][l] = osc[j]->pitchShiftFactor;
			b.oFoModLin[j][l] = osc[j]->m_dFoModLin;
			b.oFo[j][l] = osc[j]->m_dFo;
			b.oInc[j][l] = osc[j]->m_dInc;
			b.oSr[j][l] = *osc[j]->m_dSampleRate;
			b.oTri[j][l] = osc[j]->triCutoff;
			b.oMod[j][l] = osc[j]->m_dModulo;
			b.oPhaseMod[j][l] = osc[j]->m_dPhaseMod;
			b.oWave[j][l] = osc[j]->m_uWaveform;
			b.oNote[j][l] = osc[j]->note;
		}
		b.osc2out[l] = v.osc2out;
		b.osc1pure[l] = v.osc1pure;
		b.gainerTemp[l] = v.gainerTemp;
		b.o[l] = v.o;
		b.o1[l] = v.o1;
		b.o2[l] = v.o2;
		b.pan[l] = v.pan;
		b.prePan[l] = v.prePan;
		b.filterEnv[l] = v.filterEnv;
		b.tuneC[l] = v.tuneC;
		b.tuneM[l] = v.tuneM;
		b.driftPitch[l] = v.driftPitch;
		b.driftFilter[l] = v.driftFilter;
		b.driftPan[l] = v.driftPan;
		b.newInt[l] = v.newInt;
		b.newOct[l] = v.newOct;
		b.newModInt[l] = v.newModInt;
		b.newModOct[l] = v.newModOct;
		b.newNote[l] = v.newNote;
		{  // expression (singleVoiceProcessing recomputes it every sample; it is constant in a block)
			const float e = exprVel ? exprAmount * (static_cast<float>(v.vel) * (1.0f / 127.0f)) : exprAmount;
			b.exprFilter[l] = e * exprDepthFilter;
			b.exprFm[l] = e * exprDepthFm;
			b.exprWidth[l] = e * exprDepthWidth;
		}
		const CSEMFilter& f = v.m_Filter1;
		b.fQ[l] = f.qValue();
		b.fG[l] = f.g;
		b.fR[l] = f.R;
		b.fA0[l] = f.m_dAlpha0;
		b.fA[l] = f.m_dAlpha;
		b.fRho[l] = f.m_dRho;
		f.getZ(b.fZ1[l], b.fZ2[l]);
		if (filtGTableT != f.sampleInterval())
			buildFilterGTable(f);
	}

	// ---- per-block constants (parameters change between blocks only)
	const int modType = ctrlValues[MOD_TYPE_ + paramStartIdx];
	const int gainSrc = ctrlValues[GAIN_ENV_SRC + paramStartIdx];
	const int modGainSrc = ctrlValues[MOD_GAIN_ENV_SRC + paramStartIdx];
	const int beatSrc = ctrlValues[MOD_BEAT_ENV_SRC + paramStartIdx];
	const int intBase = ctrlValues[INT_ + paramStartIdx];
	const int octBase = ctrlValues[OCT_ + paramStartIdx];
	const int modIntBase = ctrlValues[MOD_INT_ + paramStartIdx];
	const int modOctBase = ctrlValues[MOD_OCT_ + paramStartIdx];
	bool lfoOn[3];
	float lfoLevel[3], lfoRate[3], lfoPhase[3];
	for (int i = 0; i < 3; i++) {
		lfoOn[i] = (lfoUsedMask & (1 << (4 + i))) != 0;
		lfoLevel[i] = (*EnvLfoCtrls[4 + i]).level;
		lfoRate[i] = (*EnvLfoCtrls[4 + i]).rate;
		lfoPhase[i] = (*EnvLfoCtrls[4 + i]).phaseStart;
	}

	// ---- the block
	for (int s = 0; s < n; s++) {
		const float* ramp = rampRows[s];
		for (int l = 0; l < L; l++) {
			auto S = [&b, l](int k) -> float& { return b.cenv[k][l]; };  // *envBank[k]

			// envelopes
			S(0) = 0.0;
			for (int k = 1; k <= 3; k++) {
				b.e[l] = b.env[k][l];
				b.env[k][l] = b.e[l] * b.cdec[k][l];
				b.cenv[k][l] += b.catt[k][l] * (b.e[l] - b.cenv[k][l]);
			}
			// LFOs (unused ones are not computed; their output is not read)
			for (int i = 0; i < 3; i++)
				if (lfoOn[i])
					b.cenv[4 + i][l] = lfoStep(b, i, l, lfoLevel[i], lfoRate[i]);

			// modulation type (doNoMod / doPhaseMod / doFreqMod / doDetuneMod)
			switch (modType) {
			case 0:
				oscUpdate(b, 1, l);
				b.osc2out[l] = oscStep(b.oMod[1][l], b.oInc[1][l], b.oPhaseMod[1][l], b.oWave[1][l], b.oFo[1][l], b.oSr[1][l], b.oTri[1][l]);
				b.oFoModLin[0][l] = 0.0;
				b.oPhaseMod[0][l] = 0.0;
				break;
			case 1:
				oscUpdate(b, 1, l);
				b.osc2out[l] = oscStep(b.oMod[1][l], b.oInc[1][l], b.oPhaseMod[1][l], b.oWave[1][l], b.oFo[1][l], b.oSr[1][l], b.oTri[1][l]);
				b.oPhaseMod[0][l] = (rsMax(0.003, rsMin(1.0, ((ramp[MOD_GAIN_RAMPINDEX]) + b.exprFm[l] + (controls[MOD_GAIN_ENV] * S(modGainSrc)))))) *  b.osc2out[l];
				b.oFoModLin[0][l] = 0.0;
				break;
			case 2:
				oscUpdate(b, 1, l);
				b.osc2out[l] = oscStep(b.oMod[1][l], b.oInc[1][l], b.oPhaseMod[1][l], b.oWave[1][l], b.oFo[1][l], b.oSr[1][l], b.oTri[1][l]);
				b.oFoModLin[0][l] = (rsMax(0.003, rsMin(1.0, ((ramp[MOD_GAIN_RAMPINDEX]) + b.exprFm[l] + (controls[MOD_GAIN_ENV] * S(modGainSrc))))) * 4) * (b.oOscFo[0][l] * b.osc2out[l]);
				b.oPhaseMod[0][l] = 0.0;
				break;
			case 3:
				b.oOscFo[1][l] = b.oOscFo[1][l] + ((bpm / 60) * ((controls[MOD_BEAT_] + static_cast<int>(controls[MOD_BEAT_ENV] * S(beatSrc) * 13)) / 4));
				oscUpdate(b, 1, l);
				b.osc2out[l] = oscStep(b.oMod[1][l], b.oInc[1][l], b.oPhaseMod[1][l], b.oWave[1][l], b.oFo[1][l], b.oSr[1][l], b.oTri[1][l]);
				b.oFoModLin[0][l] = (rsMax(0.003, rsMin(1.0, ((ramp[MOD_GAIN_RAMPINDEX]) + b.exprFm[l] + (controls[MOD_GAIN_ENV] * S(modGainSrc))))) * 8) * (b.oOscFo[0][l] * b.osc2out[l]);
				b.oPhaseMod[0][l] = 0.0;
				break;
			}

			// carrier tuning
			b.newInt[l] = intBase + static_cast<int>(controls[INT_ENV] * S(newIntEnv) * 13);
			b.newOct[l] = (octBase) + static_cast<int>((controls[OCT_ENV] * S(newOctEnv) * 6));
			if (b.newOct[l] < 1) {
				b.newOct[l] = 1;
			}
			if (b.newOct[l] > 5) {
				b.newOct[l] = 5;
			}
			b.newNote[l] = b.oNote[0][l] + (b.newInt[l] + ((b.newOct[l] - 3 )* 12.0));
			b.newNote[l] = ParamClamp2(b.newNote[l], 0, 127);
			b.tuneC[l] = (((controls[TUNE_]) + (controls[TUNE_ENV] * S(tuneCEnv)))) + (((controls[FINE_] ) + (controls[FINE_ENV] * S(fineCEnv))) / 100 );
			b.oOscFo[0][l] = ((midiFreqTable[b.newNote[l]]) + (((midiFreqTable[b.newNote[l]]) * 2) *  b.tuneC[l])) * b.driftPitch[l];
			b.oOscFo[0][l] = rsMax(OSC_FO_MIN, b.oOscFo[0][l]);
			oscUpdate(b, 0, l);
			b.oTri[0][l] = ((ramp[WIDTH_RAMPINDEX]) + b.exprWidth[l] + ((controls[WIDTH_ENV] * widthPole) * S(triEnvSrc)));
			b.oTri[0][l] = rsMax(rsMin(b.oTri[0][l], 0.9), 0.1);

			// modulator tuning (used from the next sample on)
			b.newModInt[l] = modIntBase + static_cast<int>(controls[MOD_INT_ENV] * S(newModIntEnv) * 13);
			b.newModOct[l] = (modOctBase) + static_cast<int>((controls[MOD_OCT_ENV] * S(newModOctEnv) * 13));
			b.newNote[l] = b.oNote[1][l] + (b.newModInt[l] + ((b.newModOct[l] - 6) * 12.0));
			b.newNote[l] = ParamClamp2(b.newNote[l], 1, 109);
			b.tuneM[l] = (((controls[MOD_TUNE_]) + (controls[modTuneEnv] * S(modTuneCEnv)))) + (((controls[MOD_FINE_]) + (controls[MOD_FINE_ENV] * S(modFineCEnv))) / 100);
			b.oOscFo[1][l] = ((midiFreqTable[b.newNote[l]]) + (((midiFreqTable[b.newNote[l]]) * 2) *  b.tuneM[l])) * b.driftPitch[l];
			b.oOscFo[1][l] = rsMax(OSC_FO_MIN, b.oOscFo[1][l]);
			b.oTri[1][l] = ((ramp[MOD_WIDTH_RAMPINDEX]) + b.exprWidth[l] + (((controls[MOD_WIDTH_ENV] * modWidthPole) * S(modTriEnvSrc))));
			b.oTri[1][l] = rsMax(rsMin(b.oTri[1][l], 0.9), 0.1);

			// carrier, gain
			b.osc1pure[l] = oscStep(b.oMod[0][l], b.oInc[0][l], b.oPhaseMod[0][l], b.oWave[0][l], b.oFo[0][l], b.oSr[0][l], b.oTri[0][l]);
			b.gainerTemp[l] = ParamClamp2((ramp[GAIN_RAMPINDEX]) + (controls[GAIN_ENV] * S(gainSrc)), silenceCutoff, 1.0);
			b.o[l] = b.gainerTemp[l] * (b.cenv[1][l] * (b.osc1pure[l] + (b.osc2out[l] * (ramp[MOD_OSC_GAIN_RAMPINDEX]))));

			// filter (coefficient table, CSEMFilter::setG + doFilter2)
			b.filterEnv[l] = controls[FILTER_ENV] * S(filterSrc);
			const double fx = ParamClamp2((ramp[FILTER_RAMPINDEX]) + b.filterEnv[l] + b.exprFilter[l] + b.driftFilter[l], 0.01, 1.0);
			const double pos = (fx - kFiltXMin) * filtGTableScale;
			int idx = static_cast<int>(pos);
			if (idx >= kFiltGTableSize) idx = kFiltGTableSize - 1;
			const double frac = pos - idx;
			{
				const double gg = filtGTable[idx] + (filtGTable[idx + 1] - filtGTable[idx]) * frac;
				b.fG[l] = gg;
				b.fR[l] = 1.0 / (2.0*b.fQ[l]);
				b.fA0[l] = 1.0 / (1.0 + 2.0*b.fR[l]*b.fG[l] + b.fG[l] * b.fG[l]);
				b.fA[l] = b.fG[l];
				b.fRho[l] = 2.0*b.fR[l] + b.fG[l];
			}
			{
				float& xn = b.o[l];
				double aaa = b.fA0[l] * (xn - b.fRho[l] * b.fZ1[l] - b.fZ2[l]);
				double bbb = b.fA[l] * aaa + b.fZ1[l];
				double ccc = b.fA[l] * bbb + b.fZ2[l];
				b.fZ1[l] = b.fA[l] * aaa + bbb;
				b.fZ2[l] = b.fA[l] * bbb + ccc;
				xn = ccc;
			}

			// pan
			b.pan[l] = ParamClamp2(((ramp[PAN_LR_RAMPINDEX]) + b.driftPan[l] + (ramp[PAN_LR_ENV_RAMPINDEX] * S(panEnvSrc))), -1, 1 );
			b.prePan[l] += panSmoothCoef * (b.pan[l] - b.prePan[l]);
			b.o1[l] = ((b.prePan[l] + 1.0f) / 2.0f) * b.o[l];
			b.o2[l] = ((b.prePan[l] - 1.0f) / -2.0f) * b.o[l];

			voiceRowL[b.row[l]][s] = b.o1[l];
			voiceRowR[b.row[l]][s] = b.o2[l];
		}
	}

	// ---- scatter
	for (int l = 0; l < L; l++) {
		RVoice& v = *b.voice[l];
		v.voiceEnvLfo[0].env = 0.0;
		for (int k = 0; k < 7; k++)
			v.voiceEnvLfo[k].cenv = b.cenv[k][l];
		for (int k = 1; k <= 3; k++)
			v.voiceEnvLfo[k].env = b.env[k][l];
		v.e = b.e[l];
		for (int i = 0; i < 3; i++) {
			if (!lfoOn[i])
				continue;  // untouched
			CLFO& f = v.voiceEnvLfo[4 + i];
			f.m_dAmplitude = lfoLevel[i];
			f.m_dOscFo = lfoRate[i];
			f.moduloStart = lfoPhase[i];
			f.m_dModulo = b.lMod[i][l];
			f.m_dInc = b.lInc[i][l];
			f.m_dFo = b.lFo[i][l];
			f.m_dDelayCounter = b.lDelayCounter[i][l];
			f.interimModulo = b.lInterim[i][l];
			f.sawModuloInc = b.lSawInc[i][l];
			f.chaseModuloStep = b.lChaseStep[i][l];
			f.chaseModulo = b.lChase[i][l];
			f.dOut = b.lDOut[i][l];
			f.dQPOut = 0.0;
		}
		CQBLimitedOscillator* osc[2] = {&v.WTOsc, &v.WTOsc2};
		for (int j = 0; j < 2; j++) {
			osc[j]->m_dOscFo = b.oOscFo[j][l];
			osc[j]->m_dFoModLin = b.oFoModLin[j][l];
			osc[j]->m_dFo = b.oFo[j][l];
			osc[j]->m_dInc = b.oInc[j][l];
			osc[j]->triCutoff = b.oTri[j][l];
			osc[j]->m_dModulo = b.oMod[j][l];
			osc[j]->m_dPhaseMod = b.oPhaseMod[j][l];
			// (scratch members such as doOscOut, dCalcModulo and m_dPulseWidth are rewritten before
			// they are read, so they are not stored)
		}
		v.osc2out = b.osc2out[l];
		v.osc1pure = b.osc1pure[l];
		v.gainerTemp = b.gainerTemp[l];
		v.o = b.o[l];
		v.o1 = b.o1[l];
		v.o2 = b.o2[l];
		v.pan = b.pan[l];
		v.prePan = b.prePan[l];
		v.filterEnv = b.filterEnv[l];
		v.tuneC = b.tuneC[l];
		v.tuneM = b.tuneM[l];
		v.newInt = b.newInt[l];
		v.newOct = b.newOct[l];
		v.newModInt = b.newModInt[l];
		v.newModOct = b.newModOct[l];
		v.newNote = b.newNote[l];
		v.exprFilter = b.exprFilter[l];
		v.exprFm = b.exprFm[l];
		v.exprWidth = b.exprWidth[l];
		CSEMFilter& f = v.m_Filter1;
		f.g = b.fG[l];
		f.R = b.fR[l];
		f.m_dAlpha0 = b.fA0[l];
		f.m_dAlpha = b.fA[l];
		f.m_dRho = b.fRho[l];
		f.setZ(b.fZ1[l], b.fZ2[l]);
	}
}

}  // namespace rs
}  // namespace Vst
}  // namespace Steinberg
