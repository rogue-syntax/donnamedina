// Voice bank kernel (2026-11-19, branch simd-voices; components in VoiceBank.h).
//
// renderBankGroup renders one block for up to kLanes voices: gather (RVoice -> bank), the sample
// loop with the lane loop inside it, scatter (bank -> RVoice). It is DXGenerator::
// singleVoiceProcessing, step for step: the envelopes, LFOs, oscillators and filter are the bank
// components' step functions; the generator-level parts (mod type, tuning, gain, pan) are here,
// expression for expression the same, with the envBank pointers resolved to bank.src rows and the
// member-function dispatch replaced by switches. Keep the two in step: a change to the voice path
// must be made in both places (golden test: run it with the bank on and off, NO_BANK).
#include "mdaDX10Processor.h"
#include "DXGenerator.h"
#include <cmath>

namespace Steinberg {
namespace Vst {
namespace rs {

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
		b.gather(l, v);
		// expression (singleVoiceProcessing recomputes it every sample; it is constant in a block)
		const float e = exprVel ? exprAmount * (static_cast<float>(v.vel) * (1.0f / 127.0f)) : exprAmount;
		b.exprFilter[l] = e * exprDepthFilter;
		b.exprFm[l] = e * exprDepthFm;
		b.exprWidth[l] = e * exprDepthWidth;
		if (filtGTableT != v.m_Filter1.sampleInterval())
			buildFilterGTable(v.m_Filter1);
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
	voicebank::Osc4& car = b.carrier;
	voicebank::Osc4& mod = b.modulator;

	// ---- the block
	for (int s = 0; s < n; s++) {
		const float* ramp = rampRows[s];
		for (int l = 0; l < L; l++) {
			auto S = [&b, l](int k) -> float& { return b.src.at(k, l); };  // *envBank[k]

			// envelopes
			S(0) = 0.0;
			for (int k = 1; k <= 3; k++)
				b.env[k].step(l, b.e[l], S(k));
			// LFOs (unused ones are not computed; their output is not read)
			for (int i = 0; i < 3; i++)
				if (lfoOn[i])
					S(4 + i) = b.lfo[i].step(l, lfoLevel[i], lfoRate[i]);

			// modulation type (doNoMod / doPhaseMod / doFreqMod / doDetuneMod): the modulator runs,
			// then its output modulates the carrier
			if (modType == 3)
				mod.oscFo[l] = mod.oscFo[l] + ((bpm / 60) * ((controls[MOD_BEAT_] + static_cast<int>(controls[MOD_BEAT_ENV] * S(beatSrc) * 13)) / 4));
			mod.update(l);
			b.osc2out[l] = mod.step(l);
			switch (modType) {
			case 0:
				car.foModLin[l] = 0.0;
				car.phaseMod[l] = 0.0;
				break;
			case 1:
				car.phaseMod[l] = (rsMax(0.003, rsMin(1.0, ((ramp[MOD_GAIN_RAMPINDEX]) + b.exprFm[l] + (controls[MOD_GAIN_ENV] * S(modGainSrc)))))) *  b.osc2out[l];
				car.foModLin[l] = 0.0;
				break;
			case 2:
				car.foModLin[l] = (rsMax(0.003, rsMin(1.0, ((ramp[MOD_GAIN_RAMPINDEX]) + b.exprFm[l] + (controls[MOD_GAIN_ENV] * S(modGainSrc))))) * 4) * (car.oscFo[l] * b.osc2out[l]);
				car.phaseMod[l] = 0.0;
				break;
			case 3:
				car.foModLin[l] = (rsMax(0.003, rsMin(1.0, ((ramp[MOD_GAIN_RAMPINDEX]) + b.exprFm[l] + (controls[MOD_GAIN_ENV] * S(modGainSrc))))) * 8) * (car.oscFo[l] * b.osc2out[l]);
				car.phaseMod[l] = 0.0;
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
			b.newNote[l] = car.note[l] + (b.newInt[l] + ((b.newOct[l] - 3 )* 12.0));
			b.newNote[l] = ParamClamp2(b.newNote[l], 0, 127);
			b.tuneC[l] = (((controls[TUNE_]) + (controls[TUNE_ENV] * S(tuneCEnv)))) + (((controls[FINE_] ) + (controls[FINE_ENV] * S(fineCEnv))) / 100 );
			car.oscFo[l] = ((midiFreqTable[b.newNote[l]]) + (((midiFreqTable[b.newNote[l]]) * 2) *  b.tuneC[l])) * b.driftPitch[l];
			car.oscFo[l] = rsMax(OSC_FO_MIN, car.oscFo[l]);
			car.update(l);
			car.tri[l] = ((ramp[WIDTH_RAMPINDEX]) + b.exprWidth[l] + ((controls[WIDTH_ENV] * widthPole) * S(triEnvSrc)));
			car.tri[l] = rsMax(rsMin(car.tri[l], 0.9), 0.1);

			// modulator tuning (used from the next sample on)
			b.newModInt[l] = modIntBase + static_cast<int>(controls[MOD_INT_ENV] * S(newModIntEnv) * 13);
			b.newModOct[l] = (modOctBase) + static_cast<int>((controls[MOD_OCT_ENV] * S(newModOctEnv) * 13));
			b.newNote[l] = mod.note[l] + (b.newModInt[l] + ((b.newModOct[l] - 6) * 12.0));
			b.newNote[l] = ParamClamp2(b.newNote[l], 1, 109);
			b.tuneM[l] = (((controls[MOD_TUNE_]) + (controls[modTuneEnv] * S(modTuneCEnv)))) + (((controls[MOD_FINE_]) + (controls[MOD_FINE_ENV] * S(modFineCEnv))) / 100);
			mod.oscFo[l] = ((midiFreqTable[b.newNote[l]]) + (((midiFreqTable[b.newNote[l]]) * 2) *  b.tuneM[l])) * b.driftPitch[l];
			mod.oscFo[l] = rsMax(OSC_FO_MIN, mod.oscFo[l]);
			mod.tri[l] = ((ramp[MOD_WIDTH_RAMPINDEX]) + b.exprWidth[l] + (((controls[MOD_WIDTH_ENV] * modWidthPole) * S(modTriEnvSrc))));
			mod.tri[l] = rsMax(rsMin(mod.tri[l], 0.9), 0.1);

			// carrier, gain
			b.osc1pure[l] = car.step(l);
			b.gainerTemp[l] = ParamClamp2((ramp[GAIN_RAMPINDEX]) + (controls[GAIN_ENV] * S(gainSrc)), silenceCutoff, 1.0);
			b.o[l] = b.gainerTemp[l] * (S(1) * (b.osc1pure[l] + (b.osc2out[l] * (ramp[MOD_OSC_GAIN_RAMPINDEX]))));

			// filter: coefficient from the table, then the filter
			b.filterEnv[l] = controls[FILTER_ENV] * S(filterSrc);
			const double fx = ParamClamp2((ramp[FILTER_RAMPINDEX]) + b.filterEnv[l] + b.exprFilter[l] + b.driftFilter[l], 0.01, 1.0);
			const double pos = (fx - kFiltXMin) * filtGTableScale;
			int idx = static_cast<int>(pos);
			if (idx >= kFiltGTableSize) idx = kFiltGTableSize - 1;
			const double frac = pos - idx;
			b.filter.setG(l, filtGTable[idx] + (filtGTable[idx + 1] - filtGTable[idx]) * frac);
			b.filter.process(l, b.o[l]);

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
	for (int l = 0; l < L; l++)
		b.scatter(l, *b.voice[l], lfoOn, lfoLevel, lfoRate, lfoPhase);
}

}  // namespace rs
}  // namespace Vst
}  // namespace Steinberg
