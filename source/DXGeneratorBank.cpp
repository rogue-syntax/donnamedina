// The voice path (2026-11-19, branch simd-voices; components in VoiceBank.h).
//
// renderBankGroup renders one block for up to kLanes voices: gather (RVoice -> bank), the sample
// loop, scatter (bank -> RVoice). Per voice and sample: envelopes, LFOs, the modulator and the mod
// type (none / phase / freq / detune), carrier and modulator tuning, the carrier, gain, the filter,
// pan, and the retrigger / steal fade with its restart. The envelopes, LFOs, oscillators and
// filter are the bank components' step functions; the rest is here. The modulation sources are
// rows of bank.src (RVoice::envBank was a pointer per source).
//
// Until 2026-11-19 this was DXGenerator::singleVoiceProcessing + interpolateVoice, one voice and one
// sample at a time; the arithmetic is the same (golden test: 0 to ~2e-7 difference, see
// docs/02-voices-and-dsp.md). The comments marked "per lane" give it in that scalar form.
//
// Inside a sample, each stage runs for all lanes before the next stage, mostly as one AVX2
// operation over the lanes. The lanes are independent, so the order does not change any result. A
// group with fewer than kLanes voices fills the spare lanes with a copy of lane 0; their results are
// not stored and they never touch another voice's objects.
#include "mdaDX10Processor.h"
#include "DXGenerator.h"
#include <cmath>

namespace Steinberg {
namespace Vst {
namespace rs {

using voicebank::kLanes;

void DXGenerator::renderBankGroup(int n) {
	VoiceBank& b = bank;
	const int L = b.count;
	bankVoiceSamples += static_cast<unsigned long long>(L) * n;

	// ---- gather (spare lanes: a copy of lane 0)
	auto gatherLane = [&](int l) {
		const bool spare = l >= L;
		RVoice& v = *b.voice[spare ? 0 : l];
		b.gather(l, v, spare);
		// expression (constant in a block)
		const float e = exprVel ? exprAmount * (static_cast<float>(v.vel) * (1.0f / 127.0f)) : exprAmount;
		b.exprFilter[l] = e * exprDepthFilter;
		b.exprFm[l] = e * exprDepthFm;
		b.exprWidth[l] = e * exprDepthWidth;
		if (filtGTableT != v.m_Filter1.sampleInterval())
			buildFilterGTable(v.m_Filter1);
	};
	for (int l = 0; l < kLanes; l++)
		gatherLane(l);
	const unsigned long long clock0 = sampleClock;

	// ---- per-block constants (parameters change between blocks only)
	// mod type 0..3 and modulation sources 0..6 (envBank indices), clamped: the switches only ever
	// send these, and an index outside would read outside bank.src
	auto clampTo = [](int x, int lo, int hi) { return x < lo ? lo : (x > hi ? hi : x); };
	const int modType = clampTo(ctrlValues[MOD_TYPE_ + paramStartIdx], 0, 3);
	auto row = [&](int k) { return b.src.cenv[clampTo(k, 0, 6)]; };
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
	const float* const Sgain = row(gainSrc);
	const float* const SmodGain = row(modGainSrc);
	const float* const Sbeat = row(beatSrc);
	const float* const SnewInt = row(newIntEnv);
	const float* const SnewOct = row(newOctEnv);
	const float* const StuneC = row(tuneCEnv);
	const float* const SfineC = row(fineCEnv);
	const float* const Stri = row(triEnvSrc);
	const float* const SnewModInt = row(newModIntEnv);
	const float* const SnewModOct = row(newModOctEnv);
	const float* const SmodTuneC = row(modTuneCEnv);
	const float* const SmodFineC = row(modFineCEnv);
	const float* const SmodTri = row(modTriEnvSrc);
	const float* const Sfilter = row(filterSrc);
	const float* const Span = row(panEnvSrc);
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
				b.lfo[i].stepAll(b.src.cenv[4 + i], lfoLevel[i], lfoRate[i]);

		// modulation type (doNoMod / doPhaseMod / doFreqMod / doDetuneMod): the modulator runs,
		// then its output modulates the carrier
		// Per lane:
		//   detune: modOscFo += (bpm / 60) * ((MOD_BEAT + int(MOD_BEAT_ENV * env * 13)) / 4)
		//   amount = rsMax(0.003, rsMin(1.0, MOD_GAIN ramp + exprFm + MOD_GAIN_ENV * env))
		//   phase: carrier phaseMod = amount * osc2out; freq / detune: foModLin = amount * 4 (8) * (oscFo * osc2out)
		if (modType == 3) {
			const __m128 beat = _mm_div_ps(_mm_add_ps(_mm_set1_ps(controls[MOD_BEAT_]),
			                                          _mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_mul_ps(_mm_mul_ps(_mm_set1_ps(controls[MOD_BEAT_ENV]), _mm_loadu_ps(Sbeat)), _mm_set1_ps(13.0f))))),
			                               _mm_set1_ps(4.0f));
			_mm256_storeu_pd(mod.oscFo, _mm256_add_pd(_mm256_loadu_pd(mod.oscFo), _mm256_mul_pd(_mm256_set1_pd(bpm / 60), _mm256_cvtps_pd(beat))));
		}
		mod.updateAll();
		double oscOut[kLanes];
		mod.stepAll(oscOut);
		const __m128 o2f = _mm256_cvtpd_ps(_mm256_loadu_pd(oscOut));  // double -> float
		_mm_storeu_ps(b.osc2out, o2f);
		{
			const __m256d dzero = _mm256_setzero_pd();
			if (modType == 0) {
				_mm256_storeu_pd(car.foModLin, dzero);
				_mm256_storeu_pd(car.phaseMod, dzero);
			}
			else {
				const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_set1_ps(ramp[MOD_GAIN_RAMPINDEX]), _mm_loadu_ps(b.exprFm)), _mm_mul_ps(_mm_set1_ps(controls[MOD_GAIN_ENV]), _mm_loadu_ps(SmodGain)));
				const __m256d amount = _mm256_max_pd(_mm256_min_pd(_mm256_cvtps_pd(sum), _mm256_set1_pd(1.0)), _mm256_set1_pd(0.003));
				const __m256d o2d = _mm256_cvtps_pd(o2f);
				if (modType == 1) {
					_mm256_storeu_pd(car.phaseMod, _mm256_mul_pd(amount, o2d));
					_mm256_storeu_pd(car.foModLin, dzero);
				}
				else {
					const __m256d depth = _mm256_set1_pd(modType == 2 ? 4.0 : 8.0);
					_mm256_storeu_pd(car.foModLin, _mm256_mul_pd(_mm256_mul_pd(amount, depth), _mm256_mul_pd(_mm256_loadu_pd(car.oscFo), o2d)));
					_mm256_storeu_pd(car.phaseMod, dzero);
				}
			}
		}

		// carrier tuning, then (used from the next sample on) modulator tuning, AVX2. Scalar
		// per lane:
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
		const __m128 o1f = _mm256_cvtpd_ps(_mm256_loadu_pd(oscOut));
		_mm_storeu_ps(b.osc1pure, o1f);
		// gainerTemp = ParamClamp2(GAIN ramp + GAIN_ENV * env, silenceCutoff, 1.0)
		// o = gainerTemp * (ampEnv * (osc1pure + osc2out * MOD_OSC_GAIN ramp))
		{
			const __m128 sum = _mm_add_ps(_mm_set1_ps(ramp[GAIN_RAMPINDEX]), _mm_mul_ps(_mm_set1_ps(controls[GAIN_ENV]), _mm_loadu_ps(Sgain)));
			const __m128 gt = _mm256_cvtpd_ps(_mm256_max_pd(_mm256_min_pd(_mm256_cvtps_pd(sum), _mm256_set1_pd(1.0)), _mm256_set1_pd(static_cast<double>(silenceCutoff))));
			_mm_storeu_ps(b.gainerTemp, gt);
			_mm_storeu_ps(b.o, _mm_mul_ps(gt, _mm_mul_ps(_mm_loadu_ps(Samp), _mm_add_ps(o1f, _mm_mul_ps(o2f, _mm_set1_ps(ramp[MOD_OSC_GAIN_RAMPINDEX]))))));
		}

		// filter: coefficient from the table, then the filter
		// AVX2. Per lane:
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
		// pan = ParamClamp2(PAN ramp + driftPan + PAN_ENV ramp * env, -1, 1); prePan glides to it;
		// o1 = (prePan + 1) / 2 * o, o2 = (prePan - 1) / -2 * o
		{
			const __m128 sum = _mm_add_ps(_mm_add_ps(_mm_set1_ps(ramp[PAN_LR_RAMPINDEX]), _mm_loadu_ps(b.driftPan)), _mm_mul_ps(_mm_set1_ps(ramp[PAN_LR_ENV_RAMPINDEX]), _mm_loadu_ps(Span)));
			const __m128 pan = _mm256_cvtpd_ps(_mm256_max_pd(_mm256_min_pd(_mm256_cvtps_pd(sum), _mm256_set1_pd(1.0)), _mm256_set1_pd(-1.0)));
			_mm_storeu_ps(b.pan, pan);
			__m128 pp = _mm_loadu_ps(b.prePan);
			pp = _mm_add_ps(pp, _mm_mul_ps(_mm_set1_ps(panSmoothCoef), _mm_sub_ps(pan, pp)));
			_mm_storeu_ps(b.prePan, pp);
			const __m128 o = _mm_loadu_ps(b.o);
			_mm_storeu_ps(b.o1, _mm_mul_ps(_mm_div_ps(_mm_add_ps(pp, _mm_set1_ps(1.0f)), _mm_set1_ps(2.0f)), o));
			_mm_storeu_ps(b.o2, _mm_mul_ps(_mm_div_ps(_mm_sub_ps(pp, _mm_set1_ps(1.0f)), _mm_set1_ps(-2.0f)), o));
		}

		// retrigger / steal fade (was interpolateVoice): an S-curve gain after the filter; when it ends,
		// the voice restarts (processNoteOn: key-sync retrigger) or goes silent (stolen). The restart
		// works on the RVoice, so that lane is scattered, restarted and gathered again.
		for (int l = 0; l < L; l++) {
			if (!b.fading[l])
				continue;
			const float fs = b.fadePos[l];
			const float g = fs * fs * (3.0f - 2.0f * fs);
			b.o[l] *= g;
			b.o1[l] *= g;
			b.o2[l] *= g;
			b.fadePos[l] -= b.fadeStep[l];
			if (b.fadePos[l] <= 0.0f) {
				b.fadePos[l] = 0.0f;
				b.fading[l] = false;
				RVoice& v = *b.voice[l];
				b.scatter(l, v, lfoOn, lfoLevel, lfoRate, lfoPhase);
				sampleClock = clock0 + s;  // processNoteOn stamps the restart
				for (int k = 0; k < GENRAMPEDPARAMSLENGTH; k++)  // and reads this sample's ramp levels
					rampedParams[k].level = ramp[k];
				if (v.killIt == false) {
					// end of a key-sync retrigger fade: restart (resets oscillators, LFOs and the filter; B-56)
					processNoteOn(v, &v.note, &v.vel, true);
					if (v.releasePending == true) {  // B-18: note-off arrived during the fade
						v.releasePending = false;
						releaseVoice(v);
					}
				}
				else {
					// stolen: silent from here (detectSilence drops it); clear the filter so nothing rings on
					v.voiceEnvLfo[1].env = 0.0f;
					v.voiceEnvLfo[1].cenv = 0.0f;
					v.m_Filter1.reset();
				}
				for (int k = 0; k < GENRAMPEDPARAMSLENGTH; k++)
					rampedParams[k].level = rampRows[n - 1][k];  // block end (renderBlock's state)
				const float out1 = b.o1[l], out2 = b.o2[l];  // this sample's (faded) output
				gatherLane(l);
				b.o1[l] = out1;
				b.o2[l] = out2;
			}
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
