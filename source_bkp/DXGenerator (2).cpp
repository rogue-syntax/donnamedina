#pragma once
#include "mdaDX10Processor.h"
#include "DXGenerator.h"
#include <math.h>
#include <cmath>
#include <bitset>


//enum class MYBYTE : std::uint8_t {};


namespace Steinberg {
namespace Vst {
namespace rs {

DXGenerator::DXGenerator(){
	setActive();
	activevoices = 0;
	targCdec = 0.0f;
	setCdec = 0.0f;
	syncLFO.startOscillator();
	controls[OSC_] = 0;
	//dX10Processor = dx10p;
	ratioBounds = 50.0f;
	

	//parentProcessor = parent;

	m_dFcControl = DEFAULT_FILTER_FC;
	m_dQControl = DEFAULT_FILTER_Q;
	m_uFilterKeyTrack = DEFAULT_FILTER_KEYTRACK;
	m_dFilterKeyTrackIntensity = DEFAULT_FILTER_KEYTRACK_INTENSITY;
	m_dFilterModRange = FILTER_FC_MOD_RANGE; // FILTER_FC_MOD_RANGE defined in filter.h


	filterLogScale2 = new LogScale<ParamValue>(0.0,		/* VST GUI Variable MIN */
		1.0,		/* VST GUI Variable MAX */
		80.0,		/* filter fc LOW */
		18000.0,	/* filter fc HIGH */
		0.5,		/* the value at position 0.5 will be: */
		1800.0);	/* 1800 Hz */

	m_pFilterLogParam = new LogScaleParameter<ParamValue>(USTRING("Filter fc"),
		105,
		(*filterLogScale2),
		(USTRING("Hz")));

	syncModTuneCtrl[0] = MOD_TUNE_;
	syncModTuneCtrl[1] = TUNE_;

	syncModFineCtrl[0] = MOD_FINE_;
	syncModFineCtrl[1] = FINE_;


	combineValues[0] = &DXGenerator::combineAdd;
	combineValues[1] = &DXGenerator::combineMultiply;

	modChoice[0] = &DXGenerator::doNoMod;
	modChoice[1] = &DXGenerator::doPhaseMod;
	modChoice[2] = &DXGenerator::doFreqMod;
	modChoice[3] = &DXGenerator::doDetuneMod;

	
	doRecalc[OSC_] = &DXGenerator::recalcMainOsc;
	doRecalc[WIDTH_] = &DXGenerator::recalcWidth;
	doRecalc[WIDTH_ENV] = &DXGenerator::recalcWidthEnv;
	doRecalc[WIDTH_ENV_SRC] = &DXGenerator::recalcWidthEnvSrc;
	doRecalc[TUNE_] = &DXGenerator::recalcTune;
	doRecalc[TUNE_ENV] = &DXGenerator::recalcTuneEnv;
	doRecalc[TUNE_ENV_SRC] = &DXGenerator::recalcTuneEnvSrc;
	doRecalc[FINE_] = &DXGenerator::recalcFine;
	doRecalc[FINE_ENV] = &DXGenerator::recalcFineEnv;
	doRecalc[FINE_ENV_SRC] = &DXGenerator::recalcFineEnv;

	doRecalc[OCT_] = &DXGenerator::recalcOct;
	doRecalc[OCT_ENV] = &DXGenerator::recalcOctEnv;
	doRecalc[OCT_ENV_SRC] = &DXGenerator::recalcOctEnvSrc;//12

	doRecalc[INT_] = &DXGenerator::recalcInt;
	doRecalc[INT_ENV] = &DXGenerator::recalcIntEnv;
	doRecalc[INT_ENV_SRC] = &DXGenerator::recalcIntEnvSrc;

	doRecalc[MOD_OSC_] = &DXGenerator::recalcModOsc;
	doRecalc[MOD_TYPE_] = &DXGenerator::recalcModType;

	doRecalc[MOD_GAIN_] = &DXGenerator::recalcModGain;
	doRecalc[MOD_GAIN_ENV] = &DXGenerator::recalcModGainEnv;
	doRecalc[MOD_GAIN_ENV_SRC] = &DXGenerator::recalcModGainEnvSrc;

	doRecalc[MOD_WIDTH_] = &DXGenerator::recalcModWidth;
	doRecalc[MOD_WIDTH_ENV] = &DXGenerator::recalcModWidthEnv;
	doRecalc[MOD_WIDTH_ENV_SRC] = &DXGenerator::recalcModWidthEnvSrc;


	doRecalc[MOD_TUNE_] = &DXGenerator::recalcModTune;
	doRecalc[MOD_TUNE_ENV] = &DXGenerator::recalcModTuneEnv;
	doRecalc[MOD_TUNE_ENV_SRC] = &DXGenerator::recalcModTuneEnvSrc;

	doRecalc[MOD_FINE_] = &DXGenerator::recalcModFine;
	doRecalc[MOD_FINE_ENV] = &DXGenerator::recalcModFineEnv;
	doRecalc[MOD_FINE_ENV_SRC] = &DXGenerator::recalcModFineEnvSrc;

	doRecalc[MOD_OCT_] = &DXGenerator::recalcModOct;
	doRecalc[MOD_OCT_ENV] = &DXGenerator::recalcModOctEnv;
	doRecalc[MOD_OCT_ENV_SRC] = &DXGenerator::recalcModOctEnvSrc;

	doRecalc[MOD_INT_] = &DXGenerator::recalcModInt;
	doRecalc[MOD_INT_ENV] = &DXGenerator::recalcModIntEnv;
	doRecalc[MOD_INT_ENV_SRC] = &DXGenerator::recalcModIntEnvSrc;

	doRecalc[MOD_BEAT_] = &DXGenerator::recalcModBeat;
	doRecalc[MOD_BEAT_ENV] = &DXGenerator::recalcModBeatEnv;
	doRecalc[MOD_BEAT_ENV_SRC] = &DXGenerator::recalcModBeatEnvSrc;

	
	doRecalc[GAIN_] = &DXGenerator::recalcGain;
	doRecalc[GAIN_ENV] = &DXGenerator::recalcGainEnv;
	doRecalc[GAIN_ENV_SRC] = &DXGenerator::recalcGainEnvSrc;

	doRecalc[FILTER_] = &DXGenerator::recalcFilter;
	doRecalc[FILTER_ENV] = &DXGenerator::recalcFilterEnv;
	doRecalc[FILTER_ENV_SRC] = &DXGenerator::recalcFilterEnvSrc;
	doRecalc[FILTER_Q] = &DXGenerator::recalcFilterQ;//45
	
	doRecalc[46] = &DXGenerator::doNothing;//46
	doRecalc[47] = &DXGenerator::doNothing;//48
	doRecalc[48] = &DXGenerator::doNothing;//48
	doRecalc[MOD_OSC_GAIN] = &DXGenerator::recalcModOscGain;//49
	doRecalc[SYNC_GEN_MOD_TUNE] = &DXGenerator::recalcSyncGenModTune;//50
	doRecalc[SYNC_GEN_MOD_FINE] = &DXGenerator::recalcSyncGenModFine;//50

	doRecalc[PAN_LR] = &DXGenerator::recalcPanLR;//49
	doRecalc[PAN_LR_ENV] = &DXGenerator::recalcPanLREnv;//50
	doRecalc[PAN_LR_ENV_SRC] = &DXGenerator::recalcPanLREnvSrc;//50

	for (int i = 0; i < GENRAMPEDPARAMSLENGTH; i++) {
		ParamRampObj rampedParams_i(controls[rampedList[i]]);
		rampedParams[i] = rampedParams_i;
		//rampedParams[i].level = &controls[rampedList[i]];
	}
	

	
};



int DXGenerator::doNothing() {
	return 0;
}

void DXGenerator::ParamClamp(float* n) {
	(*n) = fmax(fmin((*n), 1.0), 0.0);
}


float DXGenerator::combineMultiply(float valA, float valB) {
	return valA * valB;
}


float DXGenerator::combineAdd(float valA, float valB) {
	return valA + valB;
}

void DXGenerator::createVoice() {

	paramInc = (100 / sampleRate);
	
	syncLFO.pitchShiftFactor = 1;
	

	for (int v = 0; v < PARTVOICES; v++) {
		RVoicesMap[v].id = v;//figure
		RVoicesMap[v].voiceId = v;//which
		
		
		RVoicesMap[v].car = 0.0f;
		RVoicesMap[v].dcar = 0.0f;
		
		RVoicesMap[v].generator = genIdx;
		RVoicesMap[v].rampingUp = false;
		RVoicesMap[v].o = 0.0f;

		RVoicesMap[v].voiceEnvLfo[0].env = 0.0f;
		RVoicesMap[v].voiceEnvLfo[0].cdec = 0.00f;
		RVoicesMap[v].voiceEnvLfo[0].cenv = 0.0f;

		RVoicesMap[v].voiceEnvLfo[1].env = 1.0f;
		RVoicesMap[v].voiceEnvLfo[1].cdec = 0.99f;
		RVoicesMap[v].voiceEnvLfo[1].cenv = 1.0f;

		RVoicesMap[v].voiceEnvLfo[2].env = 1.0f;
		RVoicesMap[v].voiceEnvLfo[2].cdec = 0.99f;
		RVoicesMap[v].voiceEnvLfo[2].cenv = 1.0f;

		RVoicesMap[v].voiceEnvLfo[3].env = 1.0f;
		RVoicesMap[v].voiceEnvLfo[3].cdec = 0.99f;
		RVoicesMap[v].voiceEnvLfo[3].cenv = 1.0f;


		//for wavetables
#ifdef wtOsc
		RVoicesMap[v].WTOsc.wTTables = &WTTables;
		RVoicesMap[v].WTOsc.m_pCurrentTable = &WTTables.m_dSineTable[0];
#endif
		RVoicesMap[v].WTOsc.note = v;
		RVoicesMap[v].WTOsc2.note = v;
		RVoicesMap[v].WTOsc.isModulator = false;
		RVoicesMap[v].WTOsc.m_uWaveform = RVoicesMap[v].WTOsc.RSINE;
		RVoicesMap[v].WTOsc2.m_uWaveform = RVoicesMap[v].WTOsc.SINE;
		RVoicesMap[v].WTOsc2.isModulator = true;
		RVoicesMap[v].WTOsc.setSampleRate(&sampleRateDouble);
		RVoicesMap[v].WTOsc2.setSampleRate(&sampleRateDouble);
		
		RVoicesMap[v].WTOsc.m_dOscFo = NULL;

		RVoicesMap[v].m_Filter1.setSampleRate(sampleRate);
		RVoicesMap[v].m_Filter1.reset();

		for (int a = 0; a < 7; a++) {
			//for (int b = 0; b < 2; b++) {//make b2 the global one?
			RVoicesMap[v].envBank[a] = &RVoicesMap[v].voiceEnvLfo[a].cenv;
				//RVoicesMap[v].envBank[a][b] = &RVoicesMap[v].voiceEnvLfo[a].cenv;
			RVoicesMap[v].voiceEnvLfo[a].setSampleRate(&sampleRateDouble);
			//}
		}





	}

	
	


}






void DXGenerator::noteOn(int32* note, int32* velocity, int32* noteID) {
	

		vl = (*note);

		pushNote = false;

		notesSize = notes.size();

		

		for (int i = 0; i < notesSize; i++) {
			if (notes[i] == (*note)) {
				pushNote = true;
				RVoicesMap[vl].noteIdx = i;
				break;
			}
		}

		RVoicesMap[vl].note = (*note);
		RVoicesMap[vl].velocity = (*velocity);
		RVoicesMap[vl].vel = (*velocity);
		RVoicesMap[vl].velocity = 0.01 * (100 / (127 / (*velocity)));//10??
		RVoicesMap[vl].noteActive = true;
		
		if (POLYCOUNT == 1) {
			RVoicesMap[vl].mergedOscFo = lastOscFo;
			//portoInc = (1000.0*(RVoicesMap[vl].WTOsc.m_dOscFo - lastOscFo)) / 1000 / sampleRate;
			portoInc = .0036f;
		}
		/*
		if (pushNote == false) {
			notes.push_back((*note));
			RVoicesMap[vl].noteIdx = notesSize - 1;
			processNoteOn(RVoicesMap[vl], note, velocity);
		}
		else {
			RVoicesMap[vl].note = vl;
			processNoteOn(RVoicesMap[vl], note, velocity);
		}
		*/
		if (pushNote == false) {

			if (notesSize >= POLYCOUNT) {
				for (int i = 0; i <= notesSize - POLYCOUNT; i++) {
					retriggerVoice(RVoicesMap[notes[i]], true);// kill voice
				}
				
				notes.push_back((*note));
				RVoicesMap[vl].noteIdx = notesSize-1;
				processNoteOn(RVoicesMap[vl], note, velocity);
			}else {
				notes.push_back((*note));
				RVoicesMap[vl].noteIdx = notesSize-1;
				processNoteOn(RVoicesMap[vl], note, velocity);
			}
		}
		else {
			RVoicesMap[vl].WTOsc.m_dOscFoPrev = RVoicesMap[vl].WTOsc.m_dOscFo;
			RVoicesMap[vl].WTOsc2.m_dOscFoPrev = RVoicesMap[vl].WTOsc2.m_dOscFo;
			RVoicesMap[vl].portoInc = 0.0f;

			RVoicesMap[vl].note = vl;
			retriggerVoice(RVoicesMap[vl], false);
		}
		
	
}

void DXGenerator::retriggerVoice(RVoice& voiceObj, bool killIt) {

	voiceObj.voiceEnvLfo[1].env = voiceObj.voiceEnvLfo[1].cenv;
	voiceObj.voiceEnvLfo[1].catt = 1.0f;
	voiceObj.rampingDown = true;// change to 0,1
	voiceObj.killIt = killIt;
	saveTargValues(voiceObj, killIt);

}


void DXGenerator::saveTargValues(RVoice& voiceObj, bool killIt) {
	if (killIt == true) {
		voiceObj.voiceEnvLfo[1].envInc = -(1000.0*voiceObj.voiceEnvLfo[1].env) / 500 / sampleRate;
		voiceObj.voiceEnvLfo[4].m_dAmplitude = -(1000.0*voiceObj.voiceEnvLfo[4].m_dAmplitude) / 21 / sampleRate;
		voiceObj.voiceEnvLfo[5].m_dAmplitude = -(1000.0*voiceObj.voiceEnvLfo[5].m_dAmplitude) / 21 / sampleRate;
		voiceObj.voiceEnvLfo[6].m_dAmplitude = -(1000.0*voiceObj.voiceEnvLfo[6].m_dAmplitude) / 21 / sampleRate;
	}
	else {
		voiceObj.voiceEnvLfo[1].envInc = -(1000.0*voiceObj.voiceEnvLfo[1].env) / 21 / sampleRate;
		voiceObj.voiceEnvLfo[4].m_dAmplitude = -(1000.0*voiceObj.voiceEnvLfo[4].m_dAmplitude) / 21 / sampleRate;
		voiceObj.voiceEnvLfo[5].m_dAmplitude = -(1000.0*voiceObj.voiceEnvLfo[5].m_dAmplitude) / 21 / sampleRate;
		voiceObj.voiceEnvLfo[6].m_dAmplitude = -(1000.0*voiceObj.voiceEnvLfo[6].m_dAmplitude) / 21 / sampleRate;
	}
}
	



void DXGenerator::processNoteOn(RVoice& voiceObj, int32* note, int32* vel) {

	voiceObj.WTOsc.m_uWaveform = (int)controls[OSC_];
	voiceObj.WTOsc2.m_uWaveform = (int)controls[MOD_OSC_];
	voiceObj.WTOsc.modTypeSelector = modTypeSelectorInt;
	voiceObj.modChoice = modTypeSelectorInt;
	//REMOVE?

	
	voiceObj.note = (*note);
	voiceObj.vel = (*vel);
	voiceObj.velocity = 0.01 * (100 / (127/(*vel)));//10??
	voiceObj.WTOsc.note = (*note);
	voiceObj.WTOsc.m_uMIDINoteNumber = (*note);
	//voiceObj.WTOsc.m_dOscFo = midiFreqTable[(*note)];
	voiceObj.WTOsc.m_nOctave = controls[OCT_];//change to ctrls
	voiceObj.WTOsc.update();
	
	voiceObj.WTOsc.startOscillator();


	voiceObj.WTOsc2.m_uMIDINoteNumber = (*note);
	//voiceObj.WTOsc2.m_dOscFo = midiFreqTable[(*note)];
	voiceObj.WTOsc2.m_nOctave = controls[MOD_OCT_];
	voiceObj.WTOsc2.note = (*note);
	voiceObj.WTOsc2.update();
	voiceObj.WTOsc2.startOscillator();
	voiceObj.osc2out = 0.0;
	// do for all env/lfo
	//voiceObj.voiceEnvLfo[1].env = (0.0f + gainLevel) * volume * ((*vel));//make dynamic
	voiceObj.voiceEnvLfo[1].env = ( 1.0f) * voiceObj.velocity;//make dynamic
	voiceObj.voiceEnvLfo[1].catt = (*EnvLfoCtrls[1]).catt;
	voiceObj.voiceEnvLfo[1].cenv = 0.0003f;
	voiceObj.voiceEnvLfo[1].cdec = (*EnvLfoCtrls[1]).cdec;

	voiceObj.voiceEnvLfo[2].env = 1.0;
	voiceObj.voiceEnvLfo[2].cenv = 0.0;
	voiceObj.voiceEnvLfo[2].catt = (*EnvLfoCtrls[2]).catt;
	voiceObj.voiceEnvLfo[2].cdec = (*EnvLfoCtrls[2]).cdec;

	voiceObj.voiceEnvLfo[3].env = 1.0;
	voiceObj.voiceEnvLfo[3].cenv = 0.0;
	voiceObj.voiceEnvLfo[3].catt = (*EnvLfoCtrls[3]).catt;
	voiceObj.voiceEnvLfo[3].cdec = (*EnvLfoCtrls[3]).cdec;

	voiceObj.voiceEnvLfo[4].m_uWaveform = (*EnvLfoCtrls[4]).lfoType;
	voiceObj.voiceEnvLfo[4].triCutoff = 0.5;
	voiceObj.voiceEnvLfo[4].m_dPulseWidthControl = 50;
	voiceObj.voiceEnvLfo[4].moduloStart = (*EnvLfoCtrls[4]).phaseStart;
	voiceObj.voiceEnvLfo[4].reset();

	voiceObj.voiceEnvLfo[5].m_uWaveform = (*EnvLfoCtrls[5]).lfoType;
	voiceObj.voiceEnvLfo[5].triCutoff = 0.5;
	voiceObj.voiceEnvLfo[5].m_dPulseWidthControl = 50;
	voiceObj.voiceEnvLfo[5].moduloStart = (*EnvLfoCtrls[5]).phaseStart;
	voiceObj.voiceEnvLfo[5].reset();

	voiceObj.voiceEnvLfo[6].m_uWaveform = (*EnvLfoCtrls[6]).lfoType;
	voiceObj.voiceEnvLfo[6].triCutoff = 0.5;
	voiceObj.voiceEnvLfo[6].m_dPulseWidthControl = 50;
	voiceObj.voiceEnvLfo[6].moduloStart = (*EnvLfoCtrls[6]).phaseStart;
	voiceObj.voiceEnvLfo[6].reset();

	voiceObj.WTOsc.pitchShiftFactor = 1.0;
	voiceObj.WTOsc2.pitchShiftFactor = 1.0;

	newIntEnv = ctrlValues[INT_ENV_SRC];
	newOctEnv = ctrlValues[OCT_ENV_SRC];
	newModIntEnv = ctrlValues[MOD_INT_ENV_SRC];
	newModOctEnv = ctrlValues[MOD_OCT_ENV_SRC];
	tuneCEnv = ctrlValues[TUNE_ENV_SRC];
	fineCEnv = ctrlValues[FINE_ENV_SRC];
	modTuneCEnv = ctrlValues[MOD_TUNE_ENV_SRC];
	modFineCEnv = ctrlValues[MOD_FINE_ENV_SRC];
	squareEnvSrc = ctrlValues[WIDTH_ENV_SRC];
	triEnvSrc = ctrlValues[WIDTH_ENV_SRC];
	modSquareEnvSrc = ctrlValues[MOD_WIDTH_ENV_SRC];
	modTriEnvSrc = ctrlValues[MOD_WIDTH_ENV_SRC];
	panEnvSrc = ctrlValues[PAN_LR_ENV_SRC];
	modGainEnvSrc = ctrlValues[MOD_GAIN_ENV_SRC];

	voiceObj.intEnvPtr = voiceObj.envBank[newIntEnv];
	voiceObj.octEnvPtr = voiceObj.envBank[newOctEnv];
	voiceObj.tuneEnvPtr = voiceObj.envBank[tuneCEnv];
	voiceObj.fineEnvPtr = voiceObj.envBank[fineCEnv];
	voiceObj.modIntEnvPtr = voiceObj.envBank[newModIntEnv];
	voiceObj.modOctEnvPtr = voiceObj.envBank[newModOctEnv];
	voiceObj.modTuneEnvPtr = voiceObj.envBank[modTuneCEnv];
	voiceObj.modFineEnvPtr = voiceObj.envBank[modFineCEnv];
	voiceObj.triEnvPtr = voiceObj.envBank[triEnvSrc];
	voiceObj.panEnvPtr = voiceObj.envBank[panEnvSrc];
	voiceObj.modTriEnvPtr = voiceObj.envBank[modTriEnvSrc];
	voiceObj.modGainEnvPtr = voiceObj.envBank[modGainEnvSrc];
	voiceObj.gainEnvPtr = voiceObj.envBank[ctrlValues[GAIN_ENV_SRC + paramStartIdx]];

	filterEnvSrc = ctrlValues[FILTER_ENV_SRC];//MAKE PER GEN LIKE + PARTPARAMIDX etc
	voiceObj.filtEnvPtr = voiceObj.envBank[filterEnvSrc];
	filterSrc = static_cast<int>(ceil(filterEnvSrc / (filterEnvSrc + 1)));//can be calced outside?
	filterAddMult = (filterSrc*filterAddMultCtrl);
	voiceObj.m_Filter1.rawCtrl = controls[FILTER_];


	voiceObj.sampleCount = 0;

	//DO LFO INITS
	//Set decay release toggles
	for (int i = 1; i < 7; i++) {
		if ((*EnvLfoCtrls[i]).decayReleaseToggle < 0.5) {
			voiceObj.voiceEnvLfo[i].decayReleaseToggle = 0;
		}
		else {
			voiceObj.voiceEnvLfo[i].decayReleaseToggle = 1;
			if (voiceObj.voiceEnvLfo[i].cdec == 1) {
				///voiceObj.voiceEnvLfo[i].cdec = (*EnvLfoCtrls[i]).cdec;
			}
		}
	}
	
	//** changed (*rampedParams[FILTER_RAMPINDEX].level) to (rampedParams[FILTER_RAMPINDEX].level)
	voiceObj.m_Filter1.m_dFcControl = m_pFilterLogParam->toPlain((rampedParams[FILTER_RAMPINDEX].level));
	//voiceObj.m_Filter1.m_dFcControl = cookVSTGUIVariable(MIN_FILTER_FC, MAX_FILTER_FC, controls[FILTER_Q]);
	voiceObj.m_Filter1.m_dQControl = cookVSTGUIVariable(MIN_FILTER_Q, MAX_FILTER_Q, controls[FILTER_Q]);
	voiceObj.m_Filter1.setQControl(voiceObj.m_Filter1.m_dQControl);

	//voiceObj.m_Filter1.rawCtrl = controls[FILTER_];
	
	//voiceObj.o = 0.0f;
	
}



void DXGenerator::noteOff(int32* note, int32* noteID) {
	/*
		int dv;
		for (int i = 0; i < notes2.size(); i++) {
			if ( notes2[i].note == (*note) && notes2[i].isSustained == 1 ) {
				dv = i;
				notes2[i].isSustained = 0;

				for (int e = 1; e < 4; e++) {
					if (notes2[i].voiceEnvLfo[e].decayReleaseToggle < 0.5) {
						notes2[i].voiceEnvLfo[e].cdec = (*EnvLfoCtrls[e]).crel;
						if (notes2[i].voiceEnvLfo[e].cdec == 1.0) {
							notes2[i].voiceEnvLfo[e].cdec = 0.98f;
						}
						notes2[i].voiceEnvLfo[e].env = notes2[i].voiceEnvLfo[e].cenv;
						notes2[i].voiceEnvLfo[e].catt = 1.0f;


					}
					else {
						if (notes2[i].voiceEnvLfo[e].cdec == 1.0) {
							notes2[i].voiceEnvLfo[e].cdec = 0.98f;
						}
					}
				}

			}
		}

	*/
	
		int dv = (*note);
			//this handles the envelopes but not the notes
			//make this per env::
			for (int i = 1; i < 4; i++) {
				if (RVoicesMap[dv].voiceEnvLfo[i].decayReleaseToggle < 0.5) {
					RVoicesMap[dv].voiceEnvLfo[i].cdec = (*EnvLfoCtrls[i]).crel;
					if (RVoicesMap[dv].voiceEnvLfo[i].cdec == 1.0) {
						RVoicesMap[dv].voiceEnvLfo[i].cdec = 0.98f;
					}
					RVoicesMap[dv].voiceEnvLfo[i].env = RVoicesMap[dv].voiceEnvLfo[i].cenv;
					RVoicesMap[dv].voiceEnvLfo[i].catt = 1.0f;


				}
				else {
					if (RVoicesMap[dv].voiceEnvLfo[i].cdec == 1.0f) {
						RVoicesMap[dv].voiceEnvLfo[i].cdec = 0.9966402f;

					}
					RVoicesMap[dv].voiceEnvLfo[i].env = RVoicesMap[dv].voiceEnvLfo[i].cenv;//?
					RVoicesMap[dv].voiceEnvLfo[i].catt = 1.0f;//?
				}
			}
			RVoicesMap[dv].noteActive = false;
}


void DXGenerator::detectSilence( int32* activeVoices ) {
	/*

		for (int32 i = 0; i < notes2.size(); i++) { //for each voice

			if (notes2[i].voiceEnvLfo[1].cenv < 0.0001f && notes2[i].isSustained == 0) {
				//reset all envs
				notes2[i].voiceEnvLfo[1].env = 1.0f;
				notes2[i].voiceEnvLfo[1].cenv = 0.0f;
				//(*activeVoices) = (*activeVoices)-1;//check this
				notes2.erase(notes2.begin() + i);
				i -= 1;
				
			}

		}
		(*activeVoices) += notes2.size();
		*/
	
		
		for (int i = 0; i < notes.size(); i++) { //for each voice
			int x = notes[i];
			if (RVoicesMap[x].voiceEnvLfo[1].cenv < 0.0003f) {
				//reset all envs
				RVoicesMap[x].voiceEnvLfo[1].env = 1.0f;
				RVoicesMap[x].voiceEnvLfo[1].cenv = 0.0f;
				//(*activeVoices) = (*activeVoices)-1;//check this
				//if (RVoicesMap[x].noteActive == false) {//this just creates a repeat effect
					notes.erase(notes.begin() + i);
					i -= 1;
				//}
				

			}

		}
		(*activeVoices) += notes.size();
	
}



void DXGenerator::interpolateVoice(RVoice& voiceObj) {
	float signer = 1.0;
	if (voiceObj.voiceEnvLfo[1].env < 0.0003f ) {
		voiceObj.rampingDown = false;
		voiceObj.voiceEnvLfo[1].env = 0.0f;
		//reset all envLfo
		// not necc here?
		if (voiceObj.killIt == false) {
			processNoteOn(voiceObj, &voiceObj.note, &voiceObj.vel);
		}
		
	}
	else {
		voiceObj.voiceEnvLfo[1].env = voiceObj.voiceEnvLfo[1].env + voiceObj.voiceEnvLfo[1].envInc;

		voiceObj.voiceEnvLfo[4].m_dAmplitude = voiceObj.voiceEnvLfo[4].m_dAmplitude + voiceObj.voiceEnvLfo[4].envInc;
		voiceObj.voiceEnvLfo[5].m_dAmplitude = voiceObj.voiceEnvLfo[4].m_dAmplitude + voiceObj.voiceEnvLfo[5].envInc;
		voiceObj.voiceEnvLfo[6].m_dAmplitude = voiceObj.voiceEnvLfo[4].m_dAmplitude + voiceObj.voiceEnvLfo[6].envInc;
		//copysign(signer, voiceObj.o);
		//0.0 - voiceObj.o
		//voiceObj.o = voiceObj.o + (voiceObj.voiceEnvLfo[1].envInc * signer);
		//voiceObj.o = voiceObj.o / 2;
		//do per env / lfo
		//not necc here?
		if (voiceObj.killIt == false) {
			singleVoiceProcessing(voiceObj);
		}
		else {
			deadVoiceProcessing(voiceObj);
		}
		
	}
}



void DXGenerator::mainProcessing() {
	lastO = o;
	o = 0.0f;
	o1 = 0.0f;
	o2 = 0.0f;

	ctrlValuesINT_ = ctrlValues[INT_];
	ctrlValuesOCT_ = ctrlValues[OCT_];
	ctrlValuesMOD_INT_ = ctrlValues[MOD_INT_];
	ctrlValuesMOD_TYPE_ = ctrlValues[MOD_TYPE_];
	ctrlValuesMOD_OCT_ = ctrlValues[MOD_OCT_];
	controlsINT_ENV = controls[INT_ENV];
	controlsOCT_ENV = controls[OCT_ENV];
	controlsTUNE_ = controls[TUNE_];
	controlsTUNE_ENV = controls[TUNE_ENV];
	controlsFINE_ = controls[FINE_];
	controlsFINE_ENV = controls[FINE_ENV];
	controlsWIDTH_ENV = controls[WIDTH_ENV];
	controlsMOD_INT_ENV = controls[MOD_INT_ENV];
	controlsMOD_OCT_ENV = controls[MOD_OCT_ENV];
	controlsMOD_TUNE_ = controls[MOD_TUNE_];
	controlsMOD_TUNE_ENV = controls[MOD_TUNE_ENV];
	controlsMOD_FINE_ = controls[MOD_FINE_];
	controlsMOD_FINE_ENV = controls[MOD_FINE_ENV];
	controlsMOD_WIDTH_ENV = controls[MOD_WIDTH_ENV];
	controlsGAIN_ENV = controls[GAIN_ENV];
	controlsFILTER_ENV = controls[FILTER_ENV];
	controlsPAN_LR_ENV = controls[PAN_LR_ENV];
	controlsPAN_LR = controls[PAN_LR];
	controlsMOD_GAIN_ENV = controls[MOD_GAIN_ENV];
	controlsMOD_BEAT_ = controls[MOD_BEAT_];
	controlsMOD_BEAT_ENV = controls[MOD_BEAT_ENV];

	//PROCESS RAMPED PARAMS
	for (int i = 0; i < GENRAMPEDPARAMSLENGTH; i++) {
		rampedParams[i].processLevel();
	}
	
	rampedParamsWIDTH_RAMPINDEX_level = rampedParams[WIDTH_RAMPINDEX].level;
	rampedParamsMOD_WIDTH_RAMPINDEX_level = rampedParams[MOD_WIDTH_RAMPINDEX].level;
	rampedParamsFILTER_RAMPINDEX_level = rampedParams[FILTER_RAMPINDEX].level;
	rampedParamsGAIN_RAMPINDEX_level = rampedParams[GAIN_RAMPINDEX].level;
	rampedParamsMOD_OSC_GAIN_RAMPINDEX_level = rampedParams[MOD_OSC_GAIN_RAMPINDEX].level;
	rampedParamsPAN_LR_RAMPINDEX_level = rampedParams[PAN_LR_RAMPINDEX].level;
	rampedParamsMOD_GAIN_RAMPINDEX_level = rampedParams[MOD_GAIN_RAMPINDEX].level;
	

	//filterCtrlFreq = m_pFilterLogParam->toPlain(rampedParamsFILTER_RAMPINDEX_level);


	/*
	mainInt = ctrlValues[INT_ + paramStartIdx] + static_cast<int>(controls[INT_ENV] * 13);
	mainOct = (ctrlValues[OCT_ + paramStartIdx] - 3) + static_cast<int>(controls[OCT_ENV] * 6);
	mainTune = (controls[TUNE_] + controls[TUNE_ENV]) * 1000;
	mainFine = (controls[FINE_] + controls[FINE_ENV]) * 10;

	modInt = ctrlValues[MOD_INT_ + paramStartIdx] + static_cast<int>(controls[MOD_INT_ENV] * 13);
	modOct = (ctrlValues[MOD_OCT_ + paramStartIdx] - 3) + static_cast<int>(controls[MOD_OCT_ENV] * 17);
	modTune = (controls[MOD_TUNE_] + controls[MOD_TUNE_ENV]) * 1000;
	modFine = (controls[MOD_FINE_] + controls[MOD_FINE_ENV]) * 10;
	*/


	/*


		for (int n = 0; n < notes2.size(); n++) {
			if (notes2[n].rampingDown == true) {
				interpolateVoice(notes2[n]);
				o += notes2[n].o;
			}
			else {
				singleVoiceProcessing(&notes2[n]);
				o += notes2[n].o;
			}
			
		}
	
	*/
	//method one: have two sets of vector registers , one for re-note smoothing interpolation, the other for voice processing,
	//use branching to decide which register set the voice will be attached to,
	//if (xyz) putVoceOnInterpBus, else putVoiceonProcessBus()
	//method2:include interpolation process forallvoice processing,control numerically vi note on process
	//so interpolateVoiceSet(voiceBus), voiceProcess(processBus)
	//or use
		//#pragma loop(hint_parallel(6))
		//for (int n = 0; n < PARTVOICES; n++){

	int n = 0;
	int ns = notes.size();
	int nsMax = 8;
	int nsd = nsMax - ns;
	/*
	for (int i = 0; i < 8; i++) {
		notesTemp[i] = 0;
	}

	for (int i = 0; i < ns; i++) {
		notesTemp[i] = notes[i];
	}
	





	
		

		newInt_magnt = _mm256_set_ps( 13.0, 13.0, 13.0, 13.0, 13.0, 13.0, 13.0, 13.0 );

		newInt_env_cenv = _mm256_set_ps( (*RVoicesMap[notesTemp[n+7]].envBank[newIntEnv])
		, (*RVoicesMap[notesTemp[n+6]].envBank[newIntEnv])
		, (*RVoicesMap[notesTemp[n+5]].envBank[newIntEnv])
		, (*RVoicesMap[notesTemp[n + 4]].envBank[newIntEnv])
		, (*RVoicesMap[notesTemp[n + 3]].envBank[newIntEnv])
		, (*RVoicesMap[notesTemp[n + 2]].envBank[newIntEnv])
		, (*RVoicesMap[notesTemp[n + 1]].envBank[newIntEnv])
		, (*RVoicesMap[notesTemp[n + 0]].envBank[newIntEnv]) );

		newInt_controls_intrv_env = _mm256_set_ps( controls[INT_ENV]
		, controls[INT_ENV]
		, controls[INT_ENV]
		, controls[INT_ENV]
		, controls[INT_ENV]
		, controls[INT_ENV]
		, controls[INT_ENV]
		, controls[INT_ENV] );

		newInt_ctrlValues_intrv = _mm256_set_ps(ctrlValues[INT_]
		, ctrlValues[INT_]
		, ctrlValues[INT_]
		, ctrlValues[INT_]
		, ctrlValues[INT_]
		, ctrlValues[INT_]
		, ctrlValues[INT_]
		, ctrlValues[INT_] );


		newOct_magnt = _mm256_set_ps(6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0 );

		newOct_ctrl_offset = _mm256_set_ps(3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0);

		newOct_env_cenv = _mm256_set_ps( (*RVoicesMap[notesTemp[n + 7]].envBank[newOctEnv])
	, (*RVoicesMap[notesTemp[n + 6]].envBank[newOctEnv])
	, (*RVoicesMap[notesTemp[n + 5]].envBank[newOctEnv])
	, (*RVoicesMap[notesTemp[n + 4]].envBank[newOctEnv])
	, (*RVoicesMap[notesTemp[n + 3]].envBank[newOctEnv])
	, (*RVoicesMap[notesTemp[n + 2]].envBank[newOctEnv])
	, (*RVoicesMap[notesTemp[n + 1]].envBank[newOctEnv])
	, (*RVoicesMap[notesTemp[n + 0]].envBank[newOctEnv]) );

		newOct_controls_intrv_env = _mm256_set_ps( controls[OCT_ENV]
			, controls[OCT_ENV]
			, controls[OCT_ENV]
			, controls[OCT_ENV]
			, controls[OCT_ENV]
			, controls[OCT_ENV]
			, controls[OCT_ENV]
			, controls[OCT_ENV] );

		newOct_ctrlValues_intrv = _mm256_set_ps(ctrlValues[OCT_]
		, ctrlValues[OCT_]
		, ctrlValues[OCT_]
		, ctrlValues[OCT_]
		, ctrlValues[OCT_]
		, ctrlValues[OCT_]
		, ctrlValues[OCT_]
		, ctrlValues[OCT_] );

		

		newNote_magnt = _mm256_set_ps(12.0, 12.0, 12.0 ,12.0, 12.0, 12.0, 12.0, 12.0);

		newNote_set = _mm256_set_ps( RVoicesMap[notesTemp[n+7]].WTOsc.note
		, RVoicesMap[notesTemp[n+6]].WTOsc.note
		, RVoicesMap[notesTemp[n+5]].WTOsc.note
		, RVoicesMap[notesTemp[n+4]].WTOsc.note
		, RVoicesMap[notesTemp[n+3]].WTOsc.note
		, RVoicesMap[notesTemp[n+2]].WTOsc.note
		, RVoicesMap[notesTemp[n+1]].WTOsc.note
		, RVoicesMap[notesTemp[n+0]].WTOsc.note );

		tuneC_magnt = _mm256_set_ps( 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01);

		tuneC_env_fine_cenv = _mm256_set_ps( (*RVoicesMap[notesTemp[n+7]].envBank[fineCEnv])
		, (*RVoicesMap[notesTemp[n + 6]].envBank[fineCEnv])
		, (*RVoicesMap[notesTemp[n + 5]].envBank[fineCEnv])
		, (*RVoicesMap[notesTemp[n + 4]].envBank[fineCEnv])
		, (*RVoicesMap[notesTemp[n + 3]].envBank[fineCEnv])
		, (*RVoicesMap[notesTemp[n + 2]].envBank[fineCEnv])
		, (*RVoicesMap[notesTemp[n + 1]].envBank[fineCEnv])
		, (*RVoicesMap[notesTemp[n + 0]].envBank[fineCEnv]) );

		tuneC_env_fine_env = _mm256_set_ps( controls[FINE_ENV]
		, controls[FINE_ENV]
		, controls[FINE_ENV]
		, controls[FINE_ENV]
		, controls[FINE_ENV]
		, controls[FINE_ENV]
		, controls[FINE_ENV]
		, controls[FINE_ENV] );

		tuneC_env_fine = _mm256_set_ps( controls[FINE_]
		, controls[FINE_]
		, controls[FINE_]
		, controls[FINE_]
		, controls[FINE_]
		, controls[FINE_]
		, controls[FINE_]
		, controls[FINE_] );

		tuneC_env_tune_cenv = _mm256_set_ps( (*RVoicesMap[notesTemp[n + 7]].envBank[tuneCEnv])
	, (*RVoicesMap[notesTemp[n + 6]].envBank[tuneCEnv])
	, (*RVoicesMap[notesTemp[n + 5]].envBank[tuneCEnv])
	, (*RVoicesMap[notesTemp[n + 4]].envBank[tuneCEnv])
	, (*RVoicesMap[notesTemp[n + 3]].envBank[tuneCEnv])
	, (*RVoicesMap[notesTemp[n + 2]].envBank[tuneCEnv])
	, (*RVoicesMap[notesTemp[n + 1]].envBank[tuneCEnv])
	, (*RVoicesMap[notesTemp[n + 0]].envBank[tuneCEnv]) );

		tuneC_env_tune_env = _mm256_set_ps( controls[TUNE_ENV]
		, controls[TUNE_ENV]
		, controls[TUNE_ENV]
		, controls[TUNE_ENV]
		, controls[TUNE_ENV]
		, controls[TUNE_ENV]
		, controls[TUNE_ENV]
		, controls[TUNE_ENV] );

		tuneC_env_tune = _mm256_set_ps( controls[TUNE_]
		, controls[TUNE_]
		, controls[TUNE_]
		, controls[TUNE_]
		, controls[TUNE_]
		, controls[TUNE_]
		, controls[TUNE_]
		, controls[TUNE_] );

		newNote_oct_magnt = _mm256_set_ps( 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0 );

		newNote_min_bound = _mm256_set_ps(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

		tri_cut_env_min = _mm256_set_ps(0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1);

		tri_cut_env_max = _mm256_set_ps(0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9);

		tri_cut_env_src = _mm256_set_ps((*RVoicesMap[notesTemp[n + 7]].envBank[triEnvSrc])
			, (*RVoicesMap[notesTemp[n + 6]].envBank[triEnvSrc])
			, (*RVoicesMap[notesTemp[n + 5]].envBank[triEnvSrc])
			, (*RVoicesMap[notesTemp[n + 4]].envBank[triEnvSrc])
			, (*RVoicesMap[notesTemp[n + 3]].envBank[triEnvSrc])
			, (*RVoicesMap[notesTemp[n + 2]].envBank[triEnvSrc])
			, (*RVoicesMap[notesTemp[n + 1]].envBank[triEnvSrc])
			, (*RVoicesMap[notesTemp[n + 0]].envBank[triEnvSrc]));

		tri_cut_env = _mm256_set_ps(controls[WIDTH_ENV]
			, controls[WIDTH_ENV]
			, controls[WIDTH_ENV]
			, controls[WIDTH_ENV]
			, controls[WIDTH_ENV]
			, controls[WIDTH_ENV]
			, controls[WIDTH_ENV]
			, controls[WIDTH_ENV]);

		tri_cut_pole = _mm256_set_ps(widthPole
			, widthPole
			, widthPole
			, widthPole
			, widthPole
			, widthPole
			, widthPole
			, widthPole);

		tri_cut_ctrl = _mm256_set_ps(rampedParams[WIDTH_RAMPINDEX].level
			, rampedParams[WIDTH_RAMPINDEX].level
			, rampedParams[WIDTH_RAMPINDEX].level
			, rampedParams[WIDTH_RAMPINDEX].level
			, rampedParams[WIDTH_RAMPINDEX].level
			, rampedParams[WIDTH_RAMPINDEX].level
			, rampedParams[WIDTH_RAMPINDEX].level
			, rampedParams[WIDTH_RAMPINDEX].level);


		newModInt_magnt = _mm256_set_ps(13.0, 13.0, 13.0, 13.0, 13.0, 13.0, 13.0, 13.0);

		newModInt_env_cenv = _mm256_set_ps((*RVoicesMap[notesTemp[n + 7]].envBank[newModIntEnv])
			, (*RVoicesMap[notesTemp[n + 6]].envBank[newModIntEnv])
			, (*RVoicesMap[notesTemp[n + 5]].envBank[newModIntEnv])
			, (*RVoicesMap[notesTemp[n + 4]].envBank[newModIntEnv])
			, (*RVoicesMap[notesTemp[n + 3]].envBank[newModIntEnv])
			, (*RVoicesMap[notesTemp[n + 2]].envBank[newModIntEnv])
			, (*RVoicesMap[notesTemp[n + 1]].envBank[newModIntEnv])
			, (*RVoicesMap[notesTemp[n + 0]].envBank[newModIntEnv]));

		newModInt_controls_intrv_env = _mm256_set_ps(controls[MOD_INT_ENV]
			, controls[MOD_INT_ENV]
			, controls[MOD_INT_ENV]
			, controls[MOD_INT_ENV]
			, controls[MOD_INT_ENV]
			, controls[MOD_INT_ENV]
			, controls[MOD_INT_ENV]
			, controls[MOD_INT_ENV]);


		newModInt_ctrlValues_intrv = _mm256_set_ps(ctrlValues[MOD_INT_]
			, ctrlValues[MOD_INT_]
			, ctrlValues[MOD_INT_]
			, ctrlValues[MOD_INT_]
			, ctrlValues[MOD_INT_]
			, ctrlValues[MOD_INT_]
			, ctrlValues[MOD_INT_]
			, ctrlValues[MOD_INT_]);


		newOctMod_magnt = _mm256_set_ps(3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0);

		newOctMod_ctrl_offset = _mm256_set_ps(6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0);

		newOctMod_env_cenv = _mm256_set_ps((*RVoicesMap[notesTemp[n + 7]].envBank[newModOctEnv])
			, (*RVoicesMap[notesTemp[n + 6]].envBank[newModOctEnv])
			, (*RVoicesMap[notesTemp[n + 5]].envBank[newModOctEnv])
			, (*RVoicesMap[notesTemp[n + 4]].envBank[newModOctEnv])
			, (*RVoicesMap[notesTemp[n + 3]].envBank[newModOctEnv])
			, (*RVoicesMap[notesTemp[n + 2]].envBank[newModOctEnv])
			, (*RVoicesMap[notesTemp[n + 1]].envBank[newModOctEnv])
			, (*RVoicesMap[notesTemp[n + 0]].envBank[newModOctEnv]));

		newOctMod_controls_intrv_env = _mm256_set_ps(controls[MOD_OCT_ENV]
			, controls[MOD_OCT_ENV]
			, controls[MOD_OCT_ENV]
			, controls[MOD_OCT_ENV]
			, controls[MOD_OCT_ENV]
			, controls[MOD_OCT_ENV]
			, controls[MOD_OCT_ENV]
			, controls[MOD_OCT_ENV]);

		newOctMod_ctrlValues_intrv = _mm256_set_ps(ctrlValues[MOD_OCT_]
			, ctrlValues[MOD_OCT_]
			, ctrlValues[MOD_OCT_]
			, ctrlValues[MOD_OCT_]
			, ctrlValues[MOD_OCT_]
			, ctrlValues[MOD_OCT_]
			, ctrlValues[MOD_OCT_]
			, ctrlValues[MOD_OCT_]);


		newNoteMod_set = _mm256_set_ps(RVoicesMap[notesTemp[n + 7]].WTOsc2.note
			, RVoicesMap[notesTemp[n + 6]].WTOsc2.note
			, RVoicesMap[notesTemp[n + 5]].WTOsc2.note
			, RVoicesMap[notesTemp[n + 4]].WTOsc2.note
			, RVoicesMap[notesTemp[n + 3]].WTOsc2.note
			, RVoicesMap[notesTemp[n + 2]].WTOsc2.note
			, RVoicesMap[notesTemp[n + 1]].WTOsc2.note
			, RVoicesMap[notesTemp[n + 0]].WTOsc2.note);

		tuneCMod_magnt = _mm256_set_ps(100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0);

		tuneCMod_env_fine_cenv = _mm256_set_ps((*RVoicesMap[notesTemp[n + 7]].envBank[modFineCEnv])
			, (*RVoicesMap[notesTemp[n + 6]].envBank[modFineCEnv])
			, (*RVoicesMap[notesTemp[n + 5]].envBank[modFineCEnv])
			, (*RVoicesMap[notesTemp[n + 4]].envBank[modFineCEnv])
			, (*RVoicesMap[notesTemp[n + 3]].envBank[modFineCEnv])
			, (*RVoicesMap[notesTemp[n + 2]].envBank[modFineCEnv])
			, (*RVoicesMap[notesTemp[n + 1]].envBank[modFineCEnv])
			, (*RVoicesMap[notesTemp[n + 0]].envBank[modFineCEnv]));

		tuneCMod_env_fine_env = _mm256_set_ps(controls[MOD_FINE_ENV]
			, controls[MOD_FINE_ENV]
			, controls[MOD_FINE_ENV]
			, controls[MOD_FINE_ENV]
			, controls[MOD_FINE_ENV]
			, controls[MOD_FINE_ENV]
			, controls[MOD_FINE_ENV]
			, controls[MOD_FINE_ENV]);

		tuneCMod_env_fine = _mm256_set_ps(controls[MOD_FINE_]
			, controls[MOD_FINE_]
			, controls[MOD_FINE_]
			, controls[MOD_FINE_]
			, controls[MOD_FINE_]
			, controls[MOD_FINE_]
			, controls[MOD_FINE_]
			, controls[MOD_FINE_]);

		tuneCMod_env_tune_cenv = _mm256_set_ps((*RVoicesMap[notesTemp[n + 7]].envBank[modTuneCEnv])
			, (*RVoicesMap[notesTemp[n + 6]].envBank[modTuneCEnv])
			, (*RVoicesMap[notesTemp[n + 5]].envBank[modTuneCEnv])
			, (*RVoicesMap[notesTemp[n + 4]].envBank[modTuneCEnv])
			, (*RVoicesMap[notesTemp[n + 3]].envBank[modTuneCEnv])
			, (*RVoicesMap[notesTemp[n + 2]].envBank[modTuneCEnv])
			, (*RVoicesMap[notesTemp[n + 1]].envBank[modTuneCEnv])
			, (*RVoicesMap[notesTemp[n + 0]].envBank[modTuneCEnv]));

		tuneCMod_env_tune_env = _mm256_set_ps(controls[MOD_TUNE_ENV]
			, controls[MOD_TUNE_ENV]
			, controls[MOD_TUNE_ENV]
			, controls[MOD_TUNE_ENV]
			, controls[MOD_TUNE_ENV]
			, controls[MOD_TUNE_ENV]
			, controls[MOD_TUNE_ENV]
			, controls[MOD_TUNE_ENV]);

		tuneCMod_env_tune = _mm256_set_ps(controls[MOD_TUNE_]
			, controls[MOD_TUNE_]
			, controls[MOD_TUNE_]
			, controls[MOD_TUNE_]
			, controls[MOD_TUNE_]
			, controls[MOD_TUNE_]
			, controls[MOD_TUNE_]
			, controls[MOD_TUNE_]);

		newNoteMod_oct_magnt = _mm256_set_ps(2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0);

		newNote_min_bound = _mm256_set_ps(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

		newNoteMod_upper_bound = _mm256_set_ps(109,109,109,109,109,109,109,109);

		newNoteMod_lower_bound = _mm256_set_ps(1, 1, 1, 1, 1, 1, 1, 1);

		triMod_cut_env_src = _mm256_set_ps((*RVoicesMap[notesTemp[n + 7]].envBank[modTriEnvSrc])
			, (*RVoicesMap[notesTemp[n + 6]].envBank[modTriEnvSrc])
			, (*RVoicesMap[notesTemp[n + 5]].envBank[modTriEnvSrc])
			, (*RVoicesMap[notesTemp[n + 4]].envBank[modTriEnvSrc])
			, (*RVoicesMap[notesTemp[n + 3]].envBank[modTriEnvSrc])
			, (*RVoicesMap[notesTemp[n + 2]].envBank[modTriEnvSrc])
			, (*RVoicesMap[notesTemp[n + 1]].envBank[modTriEnvSrc])
			, (*RVoicesMap[notesTemp[n + 0]].envBank[modTriEnvSrc]));

		triMod_cut_env = _mm256_set_ps(controls[MOD_WIDTH_ENV]
			, controls[MOD_WIDTH_ENV]
			, controls[MOD_WIDTH_ENV]
			, controls[MOD_WIDTH_ENV]
			, controls[MOD_WIDTH_ENV]
			, controls[MOD_WIDTH_ENV]
			, controls[MOD_WIDTH_ENV]
			, controls[MOD_WIDTH_ENV]);

		triMod_cut_pole = _mm256_set_ps(modWidthPole
			, modWidthPole
			, modWidthPole
			, modWidthPole
			, modWidthPole
			, modWidthPole
			, modWidthPole
			, modWidthPole);

		triMod_cut_ctrl = _mm256_set_ps(rampedParams[MOD_WIDTH_RAMPINDEX].level
			, rampedParams[MOD_WIDTH_RAMPINDEX].level
			, rampedParams[MOD_WIDTH_RAMPINDEX].level
			, rampedParams[MOD_WIDTH_RAMPINDEX].level
			, rampedParams[MOD_WIDTH_RAMPINDEX].level
			, rampedParams[MOD_WIDTH_RAMPINDEX].level
			, rampedParams[MOD_WIDTH_RAMPINDEX].level
			, rampedParams[MOD_WIDTH_RAMPINDEX].level);

		*/
		


		/*--- do ops ---*/
	/*
		// voiceObj.newInt = ctrlValues[INT_ + paramStartIdx] + static_cast<int>(controls[INT_ENV] * (*voiceObj.envBank[newIntEnv][0]) * 13);
		newInt_env_cenv = _mm256_mul_ps(newInt_magnt, newInt_env_cenv);
		newInt_controls_intrv_env = _mm256_mul_ps(newInt_controls_intrv_env, newInt_env_cenv);
		newInt_ctrlValues_intrv = _mm256_add_ps(newInt_ctrlValues_intrv, newInt_controls_intrv_env);
		//newInt_ctrlValues_intrv is "newInt
		//voiceObj.newOct = (ctrlValues[OCT_ + paramStartIdx] - 3) + static_cast<int>(controls[OCT_ENV] * (*voiceObj.envBank[newOctEnv][0]) * 6);
		newOct_env_cenv = _mm256_mul_ps(newOct_env_cenv, newOct_magnt);
		newOct_controls_intrv_env = _mm256_mul_ps(newOct_controls_intrv_env, newOct_env_cenv);
		newOct_ctrlValues_intrv = _mm256_sub_ps(newOct_ctrlValues_intrv, newOct_ctrl_offset);
		newOct_ctrlValues_intrv = _mm256_add_ps(newOct_ctrlValues_intrv, newOct_controls_intrv_env);
		//newOct_ctrlValues_intrv isnewOct
		//voiceObj.newNote = voiceObj.WTOsc.note + (voiceObj.newInt + (voiceObj.newOct * 12.0));
		newOct_ctrlValues_intrv = _mm256_mul_ps(newOct_ctrlValues_intrv, newOct_magnt);
		newInt_ctrlValues_intrv = _mm256_add_ps(newInt_ctrlValues_intrv, newOct_ctrlValues_intrv);
		newNote_set = _mm256_add_ps(newNote_set, newInt_ctrlValues_intrv);
		// voiceObj.tuneC = (((controls[TUNE_]) + (controls[TUNE_ENV] * (*voiceObj.envBank[tuneCEnv][0])))) + (((controls[FINE_]) + (controls[FINE_ENV] * (*voiceObj.envBank[fineCEnv][0]))) / 100);
		tuneC_env_fine_env = _mm256_mul_ps(tuneC_env_fine_env, tuneC_env_fine_cenv);
		tuneC_env_fine = _mm256_add_ps(tuneC_env_fine, tuneC_env_fine_env);
		tuneC_env_fine = _mm256_mul_ps(tuneC_env_fine, tuneC_magnt);
		tuneC_env_tune_env = _mm256_mul_ps(tuneC_env_tune_env, tuneC_env_tune_cenv);
		tuneC_env_tune = _mm256_add_ps(tuneC_env_tune, tuneC_env_tune_env);
		tuneC_env_tune = _mm256_add_ps(tuneC_env_tune, tuneC_env_fine);
		//this is voiceobj.tuneC
		//voiceObj.WTOsc.m_dOscFo = (midiFreqTable[voiceObj.newNote]) + (((midiFreqTable[voiceObj.newNote]) * 2) *  voiceObj.tuneC);
		_mm256_storeu_ps(storeNewNote, newNoteMod_set);
		newNote_freq = _mm256_set_ps(midiFreqTable[int(storeNewNote[7])]
			, midiFreqTable[int(storeNewNote[6])]
			, midiFreqTable[int(storeNewNote[5])]
			, midiFreqTable[int(storeNewNote[4])]
			, midiFreqTable[int(storeNewNote[3])]
			, midiFreqTable[int(storeNewNote[2])]
			, midiFreqTable[int(storeNewNote[1])]
			, midiFreqTable[int(storeNewNote[0])]);
		newNote_oct_magnt = _mm256_mul_ps(newNote_freq, newNote_oct_magnt);
		newNote_oct_magnt = _mm256_mul_ps(tuneC_env_tune, newNote_oct_magnt);
		newNote_freq = _mm256_add_ps(newNote_oct_magnt, newNote_freq);
		//gotta give this to osc mdoscfo
		newNote_freq = _mm256_max_ps(newNote_min_bound, newNote_freq);
		//voiceObj.WTOsc.triCutoff = ((rampedParams[WIDTH_RAMPINDEX].level) + ((controls[WIDTH_ENV] * widthPole) * (*voiceObj.envBank[triEnvSrc][0])));
		//voiceObj.WTOsc.triCutoff = fmax(fmin(voiceObj.WTOsc.triCutoff, 0.9), 0.1);
		tri_cut_env = _mm256_mul_ps(tri_cut_env, tri_cut_pole);
		tri_cut_env = _mm256_mul_ps(tri_cut_env_src, tri_cut_env);
		tri_cut_env = _mm256_add_ps(tri_cut_ctrl, tri_cut_env);

		tri_cut_env = _mm256_min_ps(tri_cut_env, tri_cut_env_max);
		tri_cut_env = _mm256_max_ps(tri_cut_env, tri_cut_env_min);
		//gotta give this to voice obj osc WTOsc.triCutoff

		 
		//voiceObj.newModInt = ctrlValues[MOD_INT_ + paramStartIdx] + static_cast<int>(controls[MOD_INT_ENV] * (*voiceObj.envBank[newModIntEnv][0]) * 13);
		newModInt_env_cenv = _mm256_mul_ps(newModInt_magnt, newModInt_env_cenv);
		newModInt_controls_intrv_env = _mm256_mul_ps(newModInt_controls_intrv_env, newModInt_env_cenv);
		newModInt_ctrlValues_intrv = _mm256_add_ps(newModInt_ctrlValues_intrv, newModInt_controls_intrv_env);

	//voiceObj.newModOct = (ctrlValues[MOD_OCT_ + paramStartIdx]) + static_cast<int>((controls[MOD_OCT_ENV] * (*voiceObj.envBank[newModOctEnv][0]) * 17));
		newOctMod_env_cenv = _mm256_mul_ps(newOctMod_env_cenv, newOctMod_magnt);
		newOctMod_controls_intrv_env = _mm256_mul_ps(newOctMod_controls_intrv_env, newOctMod_env_cenv);
		newOctMod_ctrlValues_intrv = _mm256_sub_ps(newOctMod_ctrlValues_intrv, newOctMod_ctrl_offset);
		newOctMod_ctrlValues_intrv = _mm256_add_ps(newOctMod_ctrlValues_intrv, newOctMod_controls_intrv_env);

	//voiceObj.newNote = voiceObj.WTOsc2.note + (voiceObj.newModInt + ((voiceObj.newModOct - 6) * 12.0));
		newOctMod_ctrlValues_intrv = _mm256_mul_ps(newOctMod_ctrlValues_intrv, newOctMod_magnt);
		newModInt_ctrlValues_intrv = _mm256_add_ps(newModInt_ctrlValues_intrv, newOctMod_ctrlValues_intrv);
		newNoteMod_set = _mm256_add_ps(newNoteMod_set, newModInt_ctrlValues_intrv);

	//voiceObj.newNote = ParamClamp2(voiceObj.newNote, 1, 109);
		newNoteMod_set = _mm256_min_ps(newNoteMod_set, newNoteMod_upper_bound);
		newNoteMod_set = _mm256_max_ps(newNoteMod_set, newNoteMod_lower_bound);

	//voiceObj.tuneM = (((controls[MOD_TUNE_]) + controls[MOD_TUNE_ENV] * (*voiceObj.envBank[modTuneCEnv][0]))) + (((controls[MOD_FINE_])) + controls[MOD_FINE_ENV] * (*voiceObj.envBank[modFineCEnv][0]) / 100);
		tuneCMod_env_fine_env = _mm256_mul_ps(tuneCMod_env_fine_env, tuneCMod_env_fine_cenv);
		tuneCMod_env_fine = _mm256_add_ps(tuneCMod_env_fine, tuneCMod_env_fine_env);
		tuneCMod_env_fine = _mm256_mul_ps(tuneCMod_env_fine, tuneCMod_magnt);
		tuneCMod_env_tune_env = _mm256_mul_ps(tuneCMod_env_tune_env, tuneCMod_env_tune_cenv);
		tuneCMod_env_tune = _mm256_add_ps(tuneCMod_env_tune, tuneCMod_env_tune_env);
		tuneCMod_env_tune = _mm256_add_ps(tuneCMod_env_tune, tuneCMod_env_fine);

	//voiceObj.WTOsc2.m_dOscFo = midiFreqTable[voiceObj.newNote] + ((midiFreqTable[voiceObj.newNote] * 2) *  voiceObj.tuneM);//this isthe issuewere using the note numver instead of freq
	//voiceObj.WTOsc2.m_dOscFo = fmax(0.0, voiceObj.WTOsc2.m_dOscFo);
		_mm256_storeu_ps(storeNewNoteMod, newNoteMod_set);
		newNoteMod_freq = _mm256_set_ps(midiFreqTable[int(storeNewNoteMod[7])]
			, midiFreqTable[int(storeNewNoteMod[6])]
			, midiFreqTable[int(storeNewNoteMod[5])]
			, midiFreqTable[int(storeNewNoteMod[4])]
			, midiFreqTable[int(storeNewNoteMod[3])]
			, midiFreqTable[int(storeNewNoteMod[2])]
			, midiFreqTable[int(storeNewNoteMod[1])]
			, midiFreqTable[int(storeNewNoteMod[0])]);

		newNoteMod_oct_magnt = _mm256_mul_ps(newNoteMod_freq, newNoteMod_oct_magnt);
		newNoteMod_oct_magnt = _mm256_mul_ps(tuneCMod_env_tune, newNoteMod_oct_magnt);
		newNoteMod_freq = _mm256_add_ps(newNoteMod_oct_magnt, newNoteMod_freq);
		//gotta give this to osc mdoscfo
		newNoteMod_freq = _mm256_max_ps(newNote_min_bound, newNoteMod_freq);

	//voiceObj.WTOsc2.triCutoff = ((rampedParams[MOD_WIDTH_RAMPINDEX].level) + (((controls[MOD_WIDTH_ENV] * modWidthPole) * (*voiceObj.envBank[modTriEnvSrc][0]))));
	//voiceObj.WTOsc2.triCutoff = fmax(fmin(voiceObj.WTOsc2.triCutoff, 0.9), 0.1);
		triMod_cut_env = _mm256_mul_ps(triMod_cut_env, triMod_cut_pole);
		triMod_cut_env = _mm256_mul_ps(triMod_cut_env_src, triMod_cut_env);
		triMod_cut_env = _mm256_add_ps(triMod_cut_ctrl, triMod_cut_env);
		triMod_cut_env = _mm256_min_ps(triMod_cut_env, tri_cut_env_max);
		triMod_cut_env = _mm256_max_ps(triMod_cut_env, tri_cut_env_min);
		//gotta give this to voice obj osc WTOsc2.triCutoff
		
		_mm256_storeu_ps(storeFO, newNote_freq);
		_mm256_storeu_ps(storeModFO, newNoteMod_freq);
		_mm256_storeu_ps(storeTriCut, tri_cut_env);
		_mm256_storeu_ps(storeModTriCut, triMod_cut_env);

		*/
		
		processVoices(ns);
		
	
}

void DXGenerator::processVoices(int noteSize) {
	for (int ni = 0; ni < noteSize; ni++) {
		int i = notes[ni];
		if (RVoicesMap[i].rampingDown == true) {
			interpolateVoice(RVoicesMap[i]);
			o += RVoicesMap[i].o;
			o1 += RVoicesMap[i].o1;
			o2 += RVoicesMap[i].o2;
		}
		else {
			//RVoicesMap[i].WTOsc.m_dOscFo = storeFO[ni];
			//RVoicesMap[i].WTOsc2.m_dOscFo = storeModFO[ni];
			//RVoicesMap[i].WTOsc.triCutoff = storeTriCut[ni];
			//RVoicesMap[i].WTOsc2.triCutoff = storeModTriCut[ni];

			singleVoiceProcessing(RVoicesMap[i]);
			o += RVoicesMap[i].o;
			o1 += RVoicesMap[i].o1;
			o2 += RVoicesMap[i].o2;
		}
	}
}

void DXGenerator::singleVoiceProcessing(RVoice& voiceObj) {


	//voiceObj.sampleCount = voiceObj.sampleCount + 1;
	//update all envs
	//find a way to optimize
	voiceObj.voiceEnvLfo[0].env = 0.0;
	voiceObj.voiceEnvLfo[0].cenv = 0.0;

	voiceObj.voiceEnvLfo[1].doEnvOp();
	//voiceObj.e = voiceObj.voiceEnvLfo[1].env;
	//voiceObj.voiceEnvLfo[1].env = voiceObj.e * voiceObj.voiceEnvLfo[1].cdec;
	//voiceObj.voiceEnvLfo[1].cenv += voiceObj.voiceEnvLfo[1].catt * (voiceObj.e - voiceObj.voiceEnvLfo[1].cenv);
	
	voiceObj.voiceEnvLfo[2].doEnvOp();
	//voiceObj.e = voiceObj.voiceEnvLfo[2].env;
	//voiceObj.voiceEnvLfo[2].env = voiceObj.e * voiceObj.voiceEnvLfo[2].cdec;
	//voiceObj.voiceEnvLfo[2].cenv += voiceObj.voiceEnvLfo[2].catt * (voiceObj.e - voiceObj.voiceEnvLfo[2].cenv);
	
	voiceObj.voiceEnvLfo[3].doEnvOp();
	//voiceObj.e = voiceObj.voiceEnvLfo[3].env;
	//voiceObj.voiceEnvLfo[3].env = voiceObj.e * voiceObj.voiceEnvLfo[3].cdec;
	//voiceObj.voiceEnvLfo[3].cenv += voiceObj.voiceEnvLfo[3].catt * (voiceObj.e - voiceObj.voiceEnvLfo[3].cenv);

	
	voiceObj.voiceEnvLfo[4].m_dAmplitude = (*EnvLfoCtrls[4]).level;
	voiceObj.voiceEnvLfo[4].m_dOscFo = (*EnvLfoCtrls[4]).rate;
	voiceObj.voiceEnvLfo[4].update();
	voiceObj.voiceEnvLfo[4].cenv = voiceObj.voiceEnvLfo[4].doOscillate();
	voiceObj.voiceEnvLfo[4].moduloStart = (*EnvLfoCtrls[4]).phaseStart;

	voiceObj.voiceEnvLfo[5].m_dAmplitude = (*EnvLfoCtrls[5]).level;
	voiceObj.voiceEnvLfo[5].m_dOscFo = (*EnvLfoCtrls[5]).rate;
	voiceObj.voiceEnvLfo[5].update();
	voiceObj.voiceEnvLfo[5].cenv = voiceObj.voiceEnvLfo[5].doOscillate(); //double to float here...
	voiceObj.voiceEnvLfo[5].moduloStart = (*EnvLfoCtrls[5]).phaseStart;

	
	voiceObj.voiceEnvLfo[6].m_dAmplitude = (*EnvLfoCtrls[6]).level;
	voiceObj.voiceEnvLfo[6].m_dOscFo = (*EnvLfoCtrls[6]).rate;
	voiceObj.voiceEnvLfo[6].update();
	voiceObj.voiceEnvLfo[6].cenv = voiceObj.voiceEnvLfo[6].doOscillate();
	voiceObj.voiceEnvLfo[6].moduloStart = (*EnvLfoCtrls[6]).phaseStart;
	
	
	

	//see if we can separate th vectorizable vs non vectorizable
	//how many static cast can we get rid of?
	//how many index lookups can we precompute?
	//maybe env bank ones , choose env doesn't need to be real time
	
	voiceObj.newInt = ctrlValuesINT_ + static_cast<int>(controlsINT_ENV * (*voiceObj.intEnvPtr) * 13);
	voiceObj.newOct = (ctrlValuesOCT_  - 3) + static_cast<int>(controlsOCT_ENV * (*voiceObj.octEnvPtr) * 6);
	voiceObj.newNote = voiceObj.WTOsc.note + (voiceObj.newInt + (voiceObj.newOct * 12.0));
	voiceObj.tuneC = ((controlsTUNE_) + (controlsTUNE_ENV * (*voiceObj.tuneEnvPtr))) + (((controlsFINE_ ) + (controlsFINE_ENV * (*voiceObj.fineEnvPtr))) / 100 );
	voiceObj.WTOsc.m_dOscFo = (midiFreqTable[voiceObj.newNote]) + (((midiFreqTable[voiceObj.newNote]) * 2) *  voiceObj.tuneC);
	voiceObj.WTOsc.m_dOscFo = fmax(0.0, voiceObj.WTOsc.m_dOscFo);
	voiceObj.WTOsc.triCutoff = ((rampedParamsWIDTH_RAMPINDEX_level) + ((controlsWIDTH_ENV * widthPole) * (*voiceObj.triEnvPtr)));
	voiceObj.WTOsc.triCutoff = fmax(fmin(voiceObj.WTOsc.triCutoff, 0.9), 0.1);
	
	//DElETE ? REMOVE
	/*
	if (POLYCOUNT == 1) {
		
		//foDifference = targOscFo - mergedOscFo;
		if (std::abs(voiceObj.targOscFo - voiceObj.mergedOscFo) > 0.003f) {
			voiceObj.mergedOscFo = voiceObj.mergedOscFo + (portoInc * ((voiceObj.targOscFo - voiceObj.mergedOscFo) / std::abs(voiceObj.targOscFo - voiceObj.mergedOscFo)));//sign of x/-x = 1 or -1
			voiceObj.WTOsc.m_dOscFo = voiceObj.mergedOscFo;
		}
		else {
			voiceObj.WTOsc.m_dOscFo = voiceObj.targOscFo;
		}
		lastOscFo = voiceObj.WTOsc.m_dOscFo;
	}
	*/
	/*
	voiceObj.WTOsc.update();
	
	voiceObj.WTOsc.triCutoff = ((rampedParams[WIDTH_RAMPINDEX].level) + ((controls[WIDTH_ENV] * widthPole) * (*voiceObj.envBank[triEnvSrc][0])));
	voiceObj.WTOsc.triCutoff = fmax(fmin(voiceObj.WTOsc.triCutoff, 0.9), 0.1);
	*/


	//MODULATOR TUNING
		/*
	float* intEnvPtr;
	float* octEnvPtr;
	float* tuneEnvPtr;
	float* fineEnvPtr;
	float* modIntEnvPtr;
	float* modOctEnvPtr;
	float* modTuneEnvPtr;
	float* modFineEncPtr;
	float* modTriEnvPtr;
	float* panEnvPtr;
	float* modTriEnvPtr;
	float* modGainEnvPtr;
	*/
	
	voiceObj.newModInt = ctrlValuesMOD_INT_ + static_cast<int>(controlsMOD_INT_ENV * (*voiceObj.modIntEnvPtr) * 13);
	voiceObj.newModOct = (ctrlValuesMOD_OCT_) + static_cast<int>((controlsMOD_OCT_ENV * (*voiceObj.modOctEnvPtr) * 17));
	voiceObj.newNote = voiceObj.WTOsc2.note + (voiceObj.newModInt + ((voiceObj.newModOct - 6) * 12.0));
	voiceObj.newNote = ParamClamp2(voiceObj.newNote, 1, 109);
	voiceObj.tuneM = (((controlsMOD_TUNE_) + controlsMOD_TUNE_ENV * (*voiceObj.modTuneEnvPtr))) + (((controlsMOD_FINE_)) + controlsMOD_FINE_ENV * (*voiceObj.modFineEnvPtr) / 100);
	voiceObj.WTOsc2.m_dOscFo = midiFreqTable[voiceObj.newNote] + ((midiFreqTable[voiceObj.newNote] * 2) *  voiceObj.tuneM);
	voiceObj.WTOsc2.m_dOscFo = fmax(0.0, voiceObj.WTOsc2.m_dOscFo);
	voiceObj.WTOsc2.triCutoff = ((rampedParamsMOD_WIDTH_RAMPINDEX_level) + (((controlsMOD_WIDTH_ENV * modWidthPole) * (*voiceObj.modTriEnvPtr))));
	voiceObj.WTOsc2.triCutoff = fmax(fmin(voiceObj.WTOsc2.triCutoff, 0.9), 0.1);
	
	//(this->*modChoice[ctrlValuesMOD_TYPE_])(&voiceObj);

	
	voiceObj.WTOsc.update();
	//DO MODULATIONS 
	
	(this->*modChoice[ctrlValues[MOD_TYPE_ + paramStartIdx]])(&voiceObj);
	
	voiceObj.osc1pure = voiceObj.WTOsc.doOscillate();


	/* make || vectorized */
	voiceObj.gainerTemp = ParamClamp2((rampedParamsGAIN_RAMPINDEX_level) + (controlsGAIN_ENV * (*voiceObj.gainEnvPtr)), silenceCutoff, 1.0);
	//voiceObj.o = voiceObj.gainerTemp * (voiceObj.voiceEnvLfo[1].cenv * (voiceObj.WTOsc.doOscillate() + (voiceObj.osc2out * (rampedParams[MOD_OSC_-GAIN_RAMPINDEX].level))));
	voiceObj.o = voiceObj.gainerTemp * (voiceObj.voiceEnvLfo[1].cenv * (voiceObj.osc1pure + (voiceObj.osc2out * (rampedParamsMOD_OSC_GAIN_RAMPINDEX_level))));
	
	
	voiceObj.filterEnv = controlsFILTER_ENV * (*voiceObj.filtEnvPtr);

	voiceObj.m_Filter1.m_dFcControl = get_m_dFcControl(m_pFilterLogParam, rampedParamsFILTER_RAMPINDEX_level, voiceObj.filterEnv);
		//voiceObj.m_Filter1.m_dFcControl = m_pFilterLogParam->toPlain(ParamClamp2((rampedParamsFILTER_RAMPINDEX_level) + voiceObj.filterEnv, 0.01, 1.0));
		//voiceObj.m_Filter1.m_dFcControl = ParamClamp2(filterCtrlFreq + (filterCtrlFreq * voiceObj.filterEnv), 0.01, 1.0);
		//voiceObj.m_Filter1.m_dFcControl = filterCtrlFreq * voiceObj.filterEnv;

	voiceObj.m_Filter1.update();
	voiceObj.o = voiceObj.m_Filter1.doFilter(voiceObj.o);
	

	voiceObj.pan = ParamClamp2(controlsPAN_LR + (controlsPAN_LR_ENV * (*voiceObj.panEnvPtr)), -1, 1 );
	voiceObj.pan = voiceObj.pan  * voiceObj.voiceEnvLfo[1].cenv;
	float panDiff = voiceObj.pan - voiceObj.prePan;
	if (std::abs(panDiff) > paramInc) {
		voiceObj.prePan = voiceObj.prePan + (paramInc * (panDiff / std::abs(panDiff)));
	}
	else {
		voiceObj.prePan = voiceObj.pan;
	}
	
	voiceObj.o1 = ((voiceObj.prePan + 1) / 2) * voiceObj.o;
	voiceObj.o2 = ((voiceObj.prePan - 1) / -2) * voiceObj.o;

	
}


void DXGenerator::deadVoiceProcessing(RVoice& voiceObj) {


	voiceObj.e = voiceObj.voiceEnvLfo[1].env;
	voiceObj.voiceEnvLfo[1].env = voiceObj.e * voiceObj.voiceEnvLfo[1].cdec;
	voiceObj.voiceEnvLfo[1].cenv += voiceObj.voiceEnvLfo[1].catt * (voiceObj.e - voiceObj.voiceEnvLfo[1].cenv);

	voiceObj.WTOsc.update();


	//DO MODULATIONS

	(this->*modChoice[ctrlValues[MOD_TYPE_ + paramStartIdx]])(&voiceObj);


	//GAIN BLOCK

	voiceObj.o = voiceObj.gainerTemp * (voiceObj.voiceEnvLfo[1].cenv * voiceObj.WTOsc.doOscillate());

	voiceObj.m_Filter1.update();
	voiceObj.o = voiceObj.m_Filter1.doFilter(voiceObj.o);
	voiceObj.o1 = voiceObj.o;
	voiceObj.o2 = voiceObj.o;

}


void DXGenerator::calculateSingleOut(RVoice& voiceObj) {

}



float DXGenerator::getRelVal1D(float rangeAStart, float rangeAStop, float rangeBStart, float rangeBStop, float value ) {
	return ( rangeBStart + ( ( value / (rangeAStop - rangeAStart) )*( rangeBStop-rangeBStart) ) );
}



//got rid of divresult/ratoppimix
void DXGenerator::recalculate(const float& sampleFloat) {
	ifs = 1.0f / sampleFloat;
	for (int i = 0; i < PARTPARAMS; i++) {
		(this->*DXGenerator::doRecalc[i])();
	}

};


void DXGenerator::recalc2(int paramId) {
	(this->*DXGenerator::doRecalc[paramId])();
}




int DXGenerator::doPhaseMod(RVoice* voiceObj) {
	voiceObj->WTOsc2.update();
	voiceObj->osc2out = voiceObj->WTOsc2.doOscillate();
	//voiceObj->WTOsc.m_dPhaseMod = (fmax(0.0, fmin(1.0, (controls[MOD_GAIN_] + (controls[MOD_GAIN_ENV] * (*voiceObj->envBank[ctrlValues[MOD_GAIN_ENV_SRC + paramStartIdx]][0])))))) * ((voiceObj->WTOsc.m_dOscFo / 50) * voiceObj->WTOsc2.doOscillate());
	voiceObj->WTOsc.m_dPhaseMod = (fmax(0.0, fmin(1.0, ((rampedParamsMOD_GAIN_RAMPINDEX_level) + (controlsMOD_GAIN_ENV * (*voiceObj->modGainEnvPtr)))))) *  voiceObj->osc2out;
	return 0;
}

int DXGenerator::doFreqMod(RVoice* voiceObj) {
	voiceObj->WTOsc2.update();
	voiceObj->osc2out = voiceObj->WTOsc2.doOscillate();
	voiceObj->WTOsc.m_dFoModLin = (fmax(0.0, fmin(1.0, ((rampedParamsMOD_GAIN_RAMPINDEX_level) + (controlsMOD_GAIN_ENV * (*voiceObj->modGainEnvPtr))))) * 4) * (voiceObj->WTOsc.m_dOscFo * voiceObj->osc2out);
	return 0;
}

int DXGenerator::doDetuneMod(RVoice* voiceObj) {
	voiceObj->WTOsc2.m_dOscFo = voiceObj->WTOsc2.m_dOscFo + ((bpm / 60) * ((controlsMOD_BEAT_ + static_cast<int>(controlsMOD_BEAT_ENV * (*voiceObj->modGainEnvPtr) * 13)) / 4));
	voiceObj->WTOsc2.update();
	voiceObj->osc2out = voiceObj->WTOsc2.doOscillate();
	voiceObj->WTOsc.m_dFoModLin = (fmax(0.0, fmin(1.0, ((rampedParamsMOD_GAIN_RAMPINDEX_level) + (controlsMOD_GAIN_ENV * (*voiceObj->modGainEnvPtr))))) * 8) * (voiceObj->WTOsc.m_dOscFo * voiceObj->osc2out);
	return 0;
}

int DXGenerator::doNoMod(RVoice* voiceObj) {
	//do nothing here
	voiceObj->WTOsc2.update();
	voiceObj->osc2out = voiceObj->WTOsc2.doOscillate();
	return 0;
}



////RECALCULATORS///////////////////////////////////////////////////////////////////////////////
// stepped params come through in ctrlValues via messaging
//others accessed by psrtParams NAME_ +paramstartidx
//then we load into controls[NAME_]
int DXGenerator::recalcMainOsc() {
	//osc_int = static_cast<int>(div((*partParams[0 + paramStartIdx] * 100), 30).quot);
	int pIdx = OSC_ + paramStartIdx;
	controls[OSC_] = ctrlValues[pIdx];
	return 0;
}

int DXGenerator::recalcWidth() {
	int pIdx = WIDTH_ + paramStartIdx;
	controls[WIDTH_] = (*partParams[pIdx]);//unncess??
	modWidthPole = 1.0;
	if (controls[WIDTH_] < 0.5) {
		modWidthPole = -1.0;
	}
	modWidthEdge = modWidthPole * 0.1;
	rampedParams[WIDTH_RAMPINDEX].setLevels2((*partParams[pIdx]), 100, &sampleRate);
	return 0;
}

int DXGenerator::recalcWidthEnv() {
	int pIdx = WIDTH_ENV + paramStartIdx;
	controls[WIDTH_ENV] = 2.0 * (*partParams[pIdx]) - 1;//uni to bi polar
	//controls[WIDTH_ENV] = 100 * controls[WIDTH_ENV];
	//controls[WIDTH_ENV] = (*partParams[pIdx]);
	//rampedParams[WIDTH_ENV_RAMPINDEX].setLevels2(controls[WIDTH_ENV], 100, &sampleRate);//set smoothing
	return 0;
}

int DXGenerator::recalcWidthEnvSrc() {
	int pIdx = WIDTH_ENV_SRC + paramStartIdx;
	controls[WIDTH_ENV_SRC] = ctrlValues[pIdx];
	return 0;
}

int DXGenerator::recalcTune(){
	int pIdx = TUNE_ + paramStartIdx;
	controls[TUNE_] = (*partParams[pIdx]) - 0.5;//uni to bi polar???
	controls[INT_] = ctrlValues[INT_+paramStartIdx] + (controls[FINE_]) + (controls[TUNE_]) * 72;
	carPitchMultiplier = pitchShiftMultiplier(controls[INT_] + pitchBendMod + controls[OCT_] * 12.0 + semiToneMod + centMod / 100.0);

return 0; };


int DXGenerator::recalcTuneEnv() {
	int pIdx = TUNE_ENV + paramStartIdx;
	controls[TUNE_ENV] = 2.0 * (*partParams[pIdx]) - 1;
	//controls[TUNE_ENV] = (*partParams[pIdx]);
	return 0;
};

int DXGenerator::recalcTuneEnvSrc() {
	int pIdx = TUNE_ENV_SRC + paramStartIdx;
	controls[TUNE_ENV_SRC] = ctrlValues[pIdx];
	return 0;
};


int DXGenerator::recalcFine(){
	int pIdx = FINE_ + paramStartIdx;
	controls[FINE_] = (*partParams[pIdx]) - 0.5;
	controls[INT_] = ctrlValues[INT_+paramStartIdx] + (controls[FINE_]) + (controls[TUNE_]) * 72;
	carPitchMultiplier = pitchShiftMultiplier(controls[INT_] + pitchBendMod + controls[OCT_] * 12.0 + semiToneMod + centMod / 100.0);

	return 0; };

int DXGenerator::recalcFineEnv() {
	int pIdx = FINE_ENV + paramStartIdx;
	controls[FINE_ENV] = 2.0 * (*partParams[pIdx]) - 1;
	return 0;
};


int DXGenerator::recalcFineEnvSrc() {
	int pIdx = FINE_ENV_SRC + paramStartIdx;
	controls[FINE_ENV_SRC] = ctrlValues[pIdx];
	return 0;
};


int DXGenerator::recalcOct(){
	int pIdx = OCT_ + paramStartIdx;
	controls[OCT_] = (ctrlValues[pIdx])-3;
	carPitchMultiplier = pitchShiftMultiplier(controls[INT_] + pitchBendMod + controls[OCT_] * 12.0 + semiToneMod + centMod / 100.0);
	return 0; };


int DXGenerator::recalcOctEnv() {
	int pIdx = OCT_ENV + paramStartIdx;
	controls[OCT_ENV] = 2.0 * (*partParams[pIdx]) - 1;
	return 0;
};

int DXGenerator::recalcOctEnvSrc() {
	int pIdx = OCT_ENV_SRC + paramStartIdx;
	controls[OCT_ENV_SRC] = ctrlValues[pIdx];
	return 0;
};


int DXGenerator::recalcInt(){
	int pIdx = INT_ + paramStartIdx;
	controls[INT_] = ctrlValues[pIdx] + (controls[FINE_]) + (controls[TUNE_]) * 72;
	carPitchMultiplier = pitchShiftMultiplier(controls[INT_] + pitchBendMod + controls[OCT_] * 12.0 + semiToneMod + centMod / 100.0);
return 0; };


int DXGenerator::recalcIntEnv() {
	int pIdx = INT_ENV + paramStartIdx;
	controls[INT_ENV] = 2.0 * (*partParams[pIdx]) - 1;
	return 0;
};

int DXGenerator::recalcIntEnvSrc() {
	int pIdx = INT_ENV_SRC + paramStartIdx;
	controls[INT_ENV_SRC] = ctrlValues[pIdx];
	return 0;
};









int DXGenerator::recalcModOsc() {
	int pidx = MOD_OSC_ + paramStartIdx;
	controls[MOD_OSC_] = ctrlValues[pidx];
	return 0;
};

int DXGenerator::recalcModType() {
	int pidx = MOD_TYPE_ + paramStartIdx;
	controls[MOD_TYPE_] = ctrlValues[pidx];
	return 0;
};


int DXGenerator::recalcModWidth() {
	int pIdx = MOD_WIDTH_ + paramStartIdx;
	controls[MOD_WIDTH_] = (*partParams[pIdx]);//unncess??
	modWidthPole = 1.0;
	if (controls[MOD_WIDTH_] < 0.5) {
		modWidthPole = -1.0;
	}
	modWidthEdge = modWidthPole*0.1;
	rampedParams[MOD_WIDTH_RAMPINDEX].setLevels2((*partParams[pIdx]), 100, &sampleRate);
	return 0;
}


int DXGenerator::recalcModWidthEnv() {
	int pIdx = MOD_WIDTH_ENV + paramStartIdx;
	controls[MOD_WIDTH_ENV] = 2.0 * (*partParams[pIdx]) - 1;//uni to bi polar
	//controls[MOD_WIDTH_ENV] = 100 * controls[MOD_WIDTH_ENV];
	return 0;
}


int DXGenerator::recalcModWidthEnvSrc() {
	int pIdx = MOD_WIDTH_ENV_SRC + paramStartIdx;
	controls[MOD_WIDTH_ENV_SRC] = ctrlValues[pIdx];
	return 0;
}

int DXGenerator::recalcModGain() {
	int pIdx = MOD_GAIN_ + paramStartIdx;
	controls[MOD_GAIN_] = (*partParams[pIdx]);//do we want bipolar -100:100 or -50:50 ?
	rampedParams[MOD_GAIN_RAMPINDEX].setLevels2(controls[MOD_GAIN_], 100, &sampleRate);
	return 0;
}


int DXGenerator::recalcModGainEnv() {
	int pIdx = MOD_GAIN_ENV + paramStartIdx;
	controls[MOD_GAIN_ENV] = 2.0 * (*partParams[pIdx]) - 1;//uni to bi polar
	return 0;
}


int DXGenerator::recalcModGainEnvSrc() {
	int pIdx = MOD_GAIN_ENV_SRC + paramStartIdx;
	controls[MOD_GAIN_ENV_SRC] = ctrlValues[pIdx];
	return 0;
}




int DXGenerator::recalcModTune() {
	int pIdx = MOD_TUNE_ + paramStartIdx;
	controls[MOD_TUNE_] = 2.0 * (*partParams[pIdx]) - 1;//uni to bi?
	//controls[MOD_TUNE_] = (*partParams[pIdx]);
	controls[MOD_INT_] = ctrlValues[MOD_INT_+paramStartIdx] + (controls[MOD_FINE_]) + (controls[MOD_TUNE_]) * 72;
	carPitchMultiplier = pitchShiftMultiplier(controls[MOD_INT_] + pitchBendMod + controls[MOD_OCT_] * 12.0 + semiToneMod + centMod / 100.0);
	return 0;


};


int DXGenerator::recalcModTuneEnv() {
	int pIdx = MOD_TUNE_ENV + paramStartIdx;
	controls[MOD_TUNE_ENV] = 2.0 * (*partParams[pIdx]) - 1;
	return 0;
};


int DXGenerator::recalcModTuneEnvSrc() {
	int pIdx = MOD_TUNE_ENV_SRC + paramStartIdx;
	controls[MOD_TUNE_ENV_SRC] = ctrlValues[pIdx];
	return 0;
};


int DXGenerator::recalcModFine() {
	int pIdx = MOD_FINE_ + paramStartIdx;
	controls[MOD_FINE_] = (*partParams[pIdx]) - 0.5;
	controls[MOD_INT_] = ctrlValues[MOD_INT_+paramStartIdx] + (controls[MOD_FINE_]) + (controls[MOD_TUNE_]) * 72;
	modPitchMultiplier = pitchShiftMultiplier(controls[MOD_INT_] + pitchBendMod + controls[MOD_OCT_] * 12.0 + semiToneMod + centMod / 100.0);

	return 0;
};

int DXGenerator::recalcModFineEnv() {
	int pIdx = MOD_FINE_ENV + paramStartIdx;
	controls[MOD_FINE_ENV] = 2.0 * (*partParams[pIdx]) - 1;
	return 0;
};

int DXGenerator::recalcModFineEnvSrc() {
	int pIdx = MOD_FINE_ENV_SRC + paramStartIdx;
	controls[MOD_FINE_ENV_SRC] = ctrlValues[pIdx];
	return 0;
};


int DXGenerator::recalcModOct() {
	int pidx = MOD_OCT_ + paramStartIdx;
	controls[MOD_OCT_] = (ctrlValues[pidx]) - 6;
	modPitchMultiplier = pitchShiftMultiplier(controls[MOD_INT_] + pitchBendMod + controls[MOD_OCT_] * 12.0 + semiToneMod + centMod / 100.0);
	return 0;
};


int DXGenerator::recalcModOctEnv() {
	int pIdx = MOD_OCT_ENV + paramStartIdx;
	controls[MOD_OCT_ENV] = 2.0 * (*partParams[pIdx]) - 1;
	return 0;
};

int DXGenerator::recalcModOctEnvSrc() {
	int pIdx = MOD_OCT_ENV_SRC + paramStartIdx;
	controls[MOD_OCT_ENV_SRC] = ctrlValues[pIdx];
	return 0;
};


int DXGenerator::recalcModInt() {
	int pIdx = MOD_INT_ + paramStartIdx;
	controls[MOD_INT_] = ctrlValues[pIdx] + (controls[MOD_FINE_] - 0.5) + (controls[MOD_TUNE_] - 0.5) * 72;
	modPitchMultiplier = pitchShiftMultiplier(controls[MOD_INT_] + pitchBendMod + controls[MOD_OCT_] * 12.0 + semiToneMod + centMod / 100.0);

	return 0;
};

int DXGenerator::recalcModIntEnv() {
	int pIdx = MOD_INT_ENV + paramStartIdx;
	controls[MOD_INT_ENV] = 2.0 * (*partParams[pIdx]) - 1;
	return 0;
};

int DXGenerator::recalcModIntEnvSrc() {
	int pIdx = MOD_INT_ENV_SRC + paramStartIdx;
	controls[MOD_INT_ENV_SRC] = ctrlValues[pIdx];
	return 0;
};


int DXGenerator::recalcModBeat() {
	int pIdx = MOD_BEAT_ + paramStartIdx;
	controls[MOD_BEAT_] = ctrlValues[pIdx];
	return 0;
};

int DXGenerator::recalcModBeatEnv() {
	int pIdx = MOD_BEAT_ENV + paramStartIdx;
	controls[MOD_BEAT_ENV] = 2.0 * (*partParams[pIdx]) - 1;
	return 0;
};

int DXGenerator::recalcModBeatEnvSrc() {
	int pIdx = MOD_BEAT_ENV_SRC + paramStartIdx;
	controls[MOD_BEAT_ENV_SRC] = ctrlValues[pIdx];
	return 0;
};


int DXGenerator::recalcGain() {
	int pIdx = GAIN_ + paramStartIdx;
	controls[GAIN_] = (*partParams[pIdx]);
	rampedParams[GAIN_RAMPINDEX].setLevels2(controls[GAIN_], 100, &sampleRate);
	return 0;
};

int DXGenerator::recalcGainEnv() {
	int pIdx = GAIN_ENV + paramStartIdx;
	controls[GAIN_ENV] = 2.0 * (*partParams[pIdx]) - 1;
	//controls[GAIN_ENV] = (*partParams[pIdx]);//uni
	return 0;
};

int DXGenerator::recalcGainEnvSrc() {
	int pIdx = GAIN_ENV_SRC + paramStartIdx;
	controls[GAIN_ENV_SRC] = ctrlValues[pIdx];
	return 0;
};


int DXGenerator::recalcFilter() {
	int pIdx = FILTER_ + paramStartIdx;
	controls[FILTER_] = (*partParams[pIdx]);
	rampedParams[FILTER_RAMPINDEX].setLevels2(controls[FILTER_], 100, &sampleRate);
	return 0;
};

int DXGenerator::recalcFilterEnv() {
	int pIdx = FILTER_ENV + paramStartIdx;
	controls[FILTER_ENV] = 2.0 * (*partParams[pIdx]) - 1;
	return 0;
};

int DXGenerator::recalcFilterEnvSrc() {
	int pIdx = FILTER_ENV_SRC + paramStartIdx;
	controls[FILTER_ENV_SRC] = ctrlValues[pIdx];
	return 0;
};

int DXGenerator::recalcFilterQ() {
	int pIdx = FILTER_Q + paramStartIdx;
	controls[FILTER_Q] = (*partParams[pIdx]);
	return 0;
};

int DXGenerator::recalcModOscGain() {
	int pIdx = MOD_OSC_GAIN + paramStartIdx;
	controls[MOD_OSC_GAIN] = (*partParams[pIdx]);//do we want bipolar -100:100 or -50:50 ?
	rampedParams[MOD_OSC_GAIN_RAMPINDEX].setLevels2((*partParams[pIdx]), 100, &sampleRate);
	return 0;
};

int DXGenerator::recalcSyncGenModTune() {
	//int pIdx = SYNC_GEN_MOD_TUNE + paramStartIdx;
	//controls[SYNC_GEN_MOD_TUNE] = (*partParams[pIdx]);
	if ((*partParams[SYNC_GEN_MOD_TUNE]) > 0 ) {
		if (linkTune != 1) {
			linkTune = 1;
			parentProcessor->sendIntAttr(&CMSG_LINK_TUNE_CHAR, "0", 1);

		}
	}
	else {
		if (linkTune != 0) {
			linkTune = 0;
			parentProcessor->sendIntAttr(&CMSG_LINK_TUNE_CHAR, "0", 0);
		}
	};
	return 0;
};

int DXGenerator::recalcSyncGenModFine() {
	int pIdx = SYNC_GEN_MOD_FINE + paramStartIdx;
	controls[SYNC_GEN_MOD_FINE] = (*partParams[pIdx]);
	return 0;
};


int DXGenerator::recalcPanLR() {
	int pIdx = PAN_LR + paramStartIdx;
	controls[PAN_LR] = ((*partParams[pIdx])*2)-1.0;
	rampedParams[PAN_LR_RAMPINDEX].setLevels2(controls[PAN_LR], 100, &sampleRate);
	return 0;
}

int DXGenerator::recalcPanLREnv() {
	int pIdx = PAN_LR_ENV + paramStartIdx;
	controls[PAN_LR_ENV] = (*partParams[pIdx]);
	return 0;
}

int DXGenerator::recalcPanLREnvSrc() {
	int pIdx = PAN_LR_ENV_SRC + paramStartIdx;
	controls[PAN_LR_ENV_SRC] = ctrlValues[pIdx];
	//stepped knob / switch pulls val from ctrlValues not partparamaters
	//controls[PAN_LR_ENV_SRC] = div(static_cast<int>((*partParams[pIdx]) * 100), 14).quot;

	return 0;
}








void DXGenerator::recalculateRel(const float& sampleFloat, RVoice& voiceObj) {

	
};








void DXGenerator::setActive(){
	lfo0 = 0.0f;
	lfo1 = 1.0f; //reset LFO phase
}


}}}