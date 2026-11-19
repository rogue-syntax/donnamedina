#pragma once
// Voice bank (2026-11-19, branch simd-voices, step 2): the per-sample state of up to kLanes voices,
// column-wise (one array element per voice, "lane"), so one block of samples can be rendered for
// several voices at once. DXGenerator::renderBankGroup gathers the voices from RVoicesMap into it at
// the start of a block, runs the block, and scatters the state back. Field types match the classes
// they come from (CLFO: float; CQBLimitedOscillator: double; CSEMFilter: double), so the arithmetic
// is the same as DXGenerator::singleVoiceProcessing.

class RVoice;

struct VoiceBank {
	static const int kLanes = 4;
	int count = 0;
	RVoice* voice[kLanes] = {};
	int row[kLanes] = {};  // output row (index in notes)

	// modulation sources, as envBank: 0 = none (always 0), 1..3 envelopes, 4..6 LFOs
	float cenv[7][kLanes];
	// envelopes 1..3 (index 0 unused)
	float env[4][kLanes], catt[4][kLanes], cdec[4][kLanes];
	float e[kLanes];

	// LFOs: index k - 4 for voiceEnvLfo[k], k = 4..6 (CLFO, float)
	float lMod[3][kLanes], lInc[3][kLanes], lFo[3][kLanes], lFoRatio[3][kLanes], lPsf[3][kLanes];
	float lFoModLin[3][kLanes], lSrF[3][kLanes], lDelayCounter[3][kLanes], lDelayInc[3][kLanes];
	float lAmpMod[3][kLanes], lPw[3][kLanes], lInterim[3][kLanes], lSawInc[3][kLanes];
	float lChaseStep[3][kLanes], lSawSmooth[3][kLanes], lDOut[3][kLanes];
	double lSr[3][kLanes];
	bool lChase[3][kLanes];
	unsigned lWave[3][kLanes];

	// oscillators: 0 = carrier (WTOsc), 1 = modulator (WTOsc2) (CQBLimitedOscillator, double)
	double oOscFo[2][kLanes], oFoRatio[2][kLanes], oPsf[2][kLanes], oFoModLin[2][kLanes];
	double oFo[2][kLanes], oInc[2][kLanes], oSr[2][kLanes], oTri[2][kLanes], oMod[2][kLanes];
	double oPhaseMod[2][kLanes];
	unsigned oWave[2][kLanes];
	int oNote[2][kLanes];

	// voice (RVoice)
	float osc2out[kLanes], osc1pure[kLanes], gainerTemp[kLanes], o[kLanes], o1[kLanes], o2[kLanes];
	float pan[kLanes], prePan[kLanes], filterEnv[kLanes], tuneC[kLanes], tuneM[kLanes];
	float exprFilter[kLanes], exprFm[kLanes], exprWidth[kLanes];
	float driftPitch[kLanes], driftFilter[kLanes], driftPan[kLanes];
	int newInt[kLanes], newOct[kLanes], newModInt[kLanes], newModOct[kLanes], newNote[kLanes];

	// filter (CSEMFilter)
	double fQ[kLanes], fG[kLanes], fR[kLanes], fA0[kLanes], fA[kLanes], fRho[kLanes];
	double fZ1[kLanes], fZ2[kLanes];
};
