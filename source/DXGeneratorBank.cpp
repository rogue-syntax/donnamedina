// Voice bank kernel (2026-11-19, branch simd-voices; components in VoiceBank.h).
//
// renderBankGroup renders one block for up to kLanes voices: gather (RVoice -> bank), the sample
// loop, scatter (bank -> RVoice). It is DXGenerator::singleVoiceProcessing, step for step: the
// envelopes, LFOs, oscillators and filter are the bank components' step functions; the
// generator-level parts (mod type, tuning, gain, pan) are here, expression for expression the same,
// with the envBank pointers resolved to bank.src rows and the member-function dispatch replaced by
// switches. Keep the two in step: a change to the voice path must be made in both places (golden
// test: run it with the bank on and off, NO_BANK).
//
// Inside a sample, each stage runs for all lanes before the next stage (lane loops per stage), so a
// stage can be one SIMD operation over the lanes. The lanes are independent, so the order does not
// change any result. A group with fewer than kLanes voices fills the spare lanes with a copy of
// lane 0; their results are not stored.
#include "mdaDX10Processor.h"
#include "DXGenerator.h"
#include <cmath>

namespace Steinberg {
namespace Vst {
namespace rs {

using voicebank::kLanes;

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

	// ---- gather (spare lanes: a copy of lane 0)
	for (int l = 0; l < kLanes; l++) {
		RVoice& v = *b.voice[l < L ? l : 0];
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
	float* const S0 = b.src.cenv[0];
	const float* const Sgain = b.src.cenv[gainSrc];
	const float* const SmodGain = b.src.cenv[modGainSrc];
	const float* const Sbeat = b.src.cenv[beatSrc];
	const float* const SnewInt = b.src.cenv[newIntEnv];
	const float* const SnewOct = b.src.cenv[newOctEnv];
	const float* const StuneC = b.src.cenv[tuneCEnv];
	const float* const SfineC = b.src.cenv[fineCEnv];
	const float* const Stri = b.src.cenv[triEnvSrc];
	const float* const SnewModInt = b.src.cenv[newModIntEnv];
	const float* const SnewModOct = b.src.cenv[newModOctEnv];
	const float* const SmodTuneC = b.src.cenv[modTuneCEnv];
	const float* const SmodFineC = b.src.cenv[modFineCEnv];
	const float* const SmodTri = b.src.cenv[modTriEnvSrc];
	const float* const Sfilter = b.src.cenv[filterSrc];
	const float* const Span = b.src.cenv[panEnvSrc];
	const float* const Samp = b.src.cenv[1];  // amp envelope

	// ---- the block
	for (int s = 0; s < n; s++) {
		const float* ramp = rampRows[s];

		// envelopes
		for (int l = 0; l < kLanes; l++)
			S0[l] = 0.0;
		for (int k = 1; k <= 3; k++)
			b.env[k].stepAll(b.e, b.src.cenv[k]);
		// LFOs (unused ones are not computed; their output is not read)
		for (int i = 0; i < 3; i++)
			if (lfoOn[i])
				for (int l = 0; l < kLanes; l++)
					b.src.cenv[4 + i][l] = b.lfo[i].step(l, lfoLevel[i], lfoRate[i]);

		// modulation type (doNoMod / doPhaseMod / doFreqMod / doDetuneMod): the modulator runs,
		// then its output modulates the carrier
		if (modType == 3)
			for (int l = 0; l < kLanes; l++)
				mod.oscFo[l] = mod.oscFo[l] + ((bpm / 60) * ((controls[MOD_BEAT_] + static_cast<int>(controls[MOD_BEAT_ENV] * Sbeat[l] * 13)) / 4));
		mod.updateAll();
		double oscOut[kLanes];
		mod.stepAll(oscOut);
		_mm_storeu_ps(b.osc2out, _mm256_cvtpd_ps(_mm256_loadu_pd(oscOut)));  // double -> float
		switch (modType) {
		case 0:
			for (int l = 0; l < kLanes; l++) {
				car.foModLin[l] = 0.0;
				car.phaseMod[l] = 0.0;
			}
			break;
		case 1:
			for (int l = 0; l < kLanes; l++) {
				car.phaseMod[l] = (rsMax(0.003, rsMin(1.0, ((ramp[MOD_GAIN_RAMPINDEX]) + b.exprFm[l] + (controls[MOD_GAIN_ENV] * SmodGain[l]))))) *  b.osc2out[l];
				car.foModLin[l] = 0.0;
			}
			break;
		case 2:
			for (int l = 0; l < kLanes; l++) {
				car.foModLin[l] = (rsMax(0.003, rsMin(1.0, ((ramp[MOD_GAIN_RAMPINDEX]) + b.exprFm[l] + (controls[MOD_GAIN_ENV] * SmodGain[l])))) * 4) * (car.oscFo[l] * b.osc2out[l]);
				car.phaseMod[l] = 0.0;
			}
			break;
		case 3:
			for (int l = 0; l < kLanes; l++) {
				car.foModLin[l] = (rsMax(0.003, rsMin(1.0, ((ramp[MOD_GAIN_RAMPINDEX]) + b.exprFm[l] + (controls[MOD_GAIN_ENV] * SmodGain[l])))) * 8) * (car.oscFo[l] * b.osc2out[l]);
				car.phaseMod[l] = 0.0;
			}
			break;
		}

		// carrier tuning, then (used from the next sample on) modulator tuning, AVX2. Scalar
		// reference (singleVoiceProcessing), per lane:
		//   newInt = INT + int(INT_ENV * env * 13); newOct = OCT + int(OCT_ENV * env * 6), 1..5
		//   newNote = note + (newInt + (newOct - 3) * 12.0), clamped 0..127 (all integers: exact)
		//   tuneC = (TUNE + TUNE_ENV * env) + (FINE + FINE_ENV * env) / 100
		//   oscFo = max(20, (midi[newNote] + midi[newNote] * 2 * tuneC) * driftPitch)
		//   triCutoff = clamp(WIDTH ramp + exprWidth + WIDTH_ENV * widthPole * env, 0.1, 0.9)
		// The modulator: MOD_ values, octave * 13 and (newModOct - 6) * 12, no octave clamp, note 1..109.
		{
			const __m128 drift = _mm_loadu_ps(b.driftPitch);
			const __m128 exprW = _mm_loadu_ps(b.exprWidth);
			const __m128 two = _mm_set1_ps(2.0f), hundred = _mm_set1_ps(100.0f);
			const __m256d foMin = _mm256_set1_pd(OSC_FO_MIN), triHi = _mm256_set1_pd(0.9), triLo = _mm256_set1_pd(0.1);
			const __m128i twelve = _mm_set1_epi32(12);

			// carrier
			__m128i nInt = _mm_add_epi32(_mm_set1_epi32(intBase), _mm_cvttps_epi32(_mm_mul_ps(_mm_mul_ps(_mm_set1_ps(controls[INT_ENV]), _mm_loadu_ps(SnewInt)), _mm_set1_ps(13.0f))));
			__m128i nOct = _mm_add_epi32(_mm_set1_epi32(octBase), _mm_cvttps_epi32(_mm_mul_ps(_mm_mul_ps(_mm_set1_ps(controls[OCT_ENV]), _mm_loadu_ps(SnewOct)), _mm_set1_ps(6.0f))));
			nOct = _mm_min_epi32(_mm_max_epi32(nOct, _mm_set1_epi32(1)), _mm_set1_epi32(5));
			__m128i note = _mm_add_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i*>(car.note)),
			                             _mm_add_epi32(nInt, _mm_mullo_epi32(_mm_sub_epi32(nOct, _mm_set1_epi32(3)), twelve)));
			note = _mm_min_epi32(_mm_max_epi32(note, _mm_set1_epi32(0)), _mm_set1_epi32(127));
			_mm_storeu_si128(reinterpret_cast<__m128i*>(b.newInt), nInt);
			_mm_storeu_si128(reinterpret_cast<__m128i*>(b.newOct), nOct);
			__m128 tune = _mm_add_ps(_mm_add_ps(_mm_set1_ps(controls[TUNE_]), _mm_mul_ps(_mm_set1_ps(controls[TUNE_ENV]), _mm_loadu_ps(StuneC))),
			                         _mm_div_ps(_mm_add_ps(_mm_set1_ps(controls[FINE_]), _mm_mul_ps(_mm_set1_ps(controls[FINE_ENV]), _mm_loadu_ps(SfineC))), hundred));
			_mm_storeu_ps(b.tuneC, tune);
			__m128 midi = _mm_i32gather_ps(midiFreqTable, note, 4);
			__m128 fo = _mm_mul_ps(_mm_add_ps(midi, _mm_mul_ps(_mm_mul_ps(midi, two), tune)), drift);
			_mm256_storeu_pd(car.oscFo, _mm256_max_pd(_mm256_cvtps_pd(fo), foMin));  // rsMax(OSC_FO_MIN, x)
			car.updateAll();
			__m128 tri = _mm_add_ps(_mm_add_ps(_mm_set1_ps(ramp[WIDTH_RAMPINDEX]), exprW),
			                        _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(controls[WIDTH_ENV]), _mm_set1_ps(widthPole)), _mm_loadu_ps(Stri)));
			_mm256_storeu_pd(car.tri, _mm256_max_pd(_mm256_min_pd(_mm256_cvtps_pd(tri), triHi), triLo));  // rsMax(rsMin(x, 0.9), 0.1)

			// modulator
			nInt = _mm_add_epi32(_mm_set1_epi32(modIntBase), _mm_cvttps_epi32(_mm_mul_ps(_mm_mul_ps(_mm_set1_ps(controls[MOD_INT_ENV]), _mm_loadu_ps(SnewModInt)), _mm_set1_ps(13.0f))));
			nOct = _mm_add_epi32(_mm_set1_epi32(modOctBase), _mm_cvttps_epi32(_mm_mul_ps(_mm_mul_ps(_mm_set1_ps(controls[MOD_OCT_ENV]), _mm_loadu_ps(SnewModOct)), _mm_set1_ps(13.0f))));
			note = _mm_add_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i*>(mod.note)),
			                     _mm_add_epi32(nInt, _mm_mullo_epi32(_mm_sub_epi32(nOct, _mm_set1_epi32(6)), twelve)));
			note = _mm_min_epi32(_mm_max_epi32(note, _mm_set1_epi32(1)), _mm_set1_epi32(109));
			_mm_storeu_si128(reinterpret_cast<__m128i*>(b.newModInt), nInt);
			_mm_storeu_si128(reinterpret_cast<__m128i*>(b.newModOct), nOct);
			_mm_storeu_si128(reinterpret_cast<__m128i*>(b.newNote), note);  // RVoice::newNote ends as the modulator's
			tune = _mm_add_ps(_mm_add_ps(_mm_set1_ps(controls[MOD_TUNE_]), _mm_mul_ps(_mm_set1_ps(controls[modTuneEnv]), _mm_loadu_ps(SmodTuneC))),
			                  _mm_div_ps(_mm_add_ps(_mm_set1_ps(controls[MOD_FINE_]), _mm_mul_ps(_mm_set1_ps(controls[MOD_FINE_ENV]), _mm_loadu_ps(SmodFineC))), hundred));
			_mm_storeu_ps(b.tuneM, tune);
			midi = _mm_i32gather_ps(midiFreqTable, note, 4);
			fo = _mm_mul_ps(_mm_add_ps(midi, _mm_mul_ps(_mm_mul_ps(midi, two), tune)), drift);
			_mm256_storeu_pd(mod.oscFo, _mm256_max_pd(_mm256_cvtps_pd(fo), foMin));
			tri = _mm_add_ps(_mm_add_ps(_mm_set1_ps(ramp[MOD_WIDTH_RAMPINDEX]), exprW),
			                 _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(controls[MOD_WIDTH_ENV]), _mm_set1_ps(modWidthPole)), _mm_loadu_ps(SmodTri)));
			_mm256_storeu_pd(mod.tri, _mm256_max_pd(_mm256_min_pd(_mm256_cvtps_pd(tri), triHi), triLo));
		}

		// carrier, gain
		car.stepAll(oscOut);
		_mm_storeu_ps(b.osc1pure, _mm256_cvtpd_ps(_mm256_loadu_pd(oscOut)));
		for (int l = 0; l < kLanes; l++) {
			b.gainerTemp[l] = ParamClamp2((ramp[GAIN_RAMPINDEX]) + (controls[GAIN_ENV] * Sgain[l]), silenceCutoff, 1.0);
			b.o[l] = b.gainerTemp[l] * (Samp[l] * (b.osc1pure[l] + (b.osc2out[l] * (ramp[MOD_OSC_GAIN_RAMPINDEX]))));
		}

		// filter: coefficient from the table, then the filter
		// AVX2. Scalar reference, per lane:
		//   filterEnv = FILTER_ENV * env
		//   fx = ParamClamp2(FILTER ramp + filterEnv + exprFilter + driftFilter, 0.01, 1.0)  (float)
		//   pos = (fx - kFiltXMin) * filtGTableScale; idx = int(pos), at most size - 1
		//   g = table[idx] + (table[idx + 1] - table[idx]) * (pos - idx)
		{
			const __m128 fEnv = _mm_mul_ps(_mm_set1_ps(controls[FILTER_ENV]), _mm_loadu_ps(Sfilter));
			_mm_storeu_ps(b.filterEnv, fEnv);
			const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_set1_ps(ramp[FILTER_RAMPINDEX]), fEnv), _mm_loadu_ps(b.exprFilter)), _mm_loadu_ps(b.driftFilter));
			// ParamClamp2 takes float limits: 0.01 becomes 0.01f; the result is a float again
			const __m256d fx = _mm256_cvtps_pd(_mm256_cvtpd_ps(_mm256_max_pd(_mm256_min_pd(_mm256_cvtps_pd(sum), _mm256_set1_pd(1.0)), _mm256_set1_pd(static_cast<double>(0.01f)))));
			const __m256d pos = _mm256_mul_pd(_mm256_sub_pd(fx, _mm256_set1_pd(kFiltXMin)), _mm256_set1_pd(filtGTableScale));
			const __m128i idx = _mm_min_epi32(_mm256_cvttpd_epi32(pos), _mm_set1_epi32(kFiltGTableSize - 1));
			const __m256d frac = _mm256_sub_pd(pos, _mm256_cvtepi32_pd(idx));
			const __m256d g0 = _mm256_i32gather_pd(filtGTable, idx, 8);
			const __m256d g1 = _mm256_i32gather_pd(filtGTable + 1, idx, 8);
			b.filter.setGAll(_mm256_add_pd(g0, _mm256_mul_pd(_mm256_sub_pd(g1, g0), frac)));
		}
		b.filter.processAll(b.o);

		// pan
		for (int l = 0; l < kLanes; l++) {
			b.pan[l] = ParamClamp2(((ramp[PAN_LR_RAMPINDEX]) + b.driftPan[l] + (ramp[PAN_LR_ENV_RAMPINDEX] * Span[l])), -1, 1 );
			b.prePan[l] += panSmoothCoef * (b.pan[l] - b.prePan[l]);
			b.o1[l] = ((b.prePan[l] + 1.0f) / 2.0f) * b.o[l];
			b.o2[l] = ((b.prePan[l] - 1.0f) / -2.0f) * b.o[l];
		}

		for (int l = 0; l < L; l++) {
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
