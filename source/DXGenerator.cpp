#pragma once
#include "mdaDX10Processor.h"
#include "DXGenerator.h"
#include <math.h>
#include <cmath>
#include <bitset>
#include <malloc.h>  // For _aligned_malloc


//enum class MYBYTE : std::uint8_t {};


namespace Steinberg {
namespace Vst {
namespace rs {

//need to clean up
//ddMap
	DXGenerator::DXGenerator() {};

	void DXGenerator::init() 
		{
			setActive();
			activevoices = 0;
			targCdec = 0.0f;
			setCdec = 0.0f;
			syncLFO.startOscillator();
			controls[OSC_] = 0;
			//dX10Processor = dx10p;
			ratioBounds = 50.0f;

			RVoicesMap = new RVoice[PARTVOICES];

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

			/*
			double dd = 0.001;
			for (int i = 0; i < 1000; i++) {
				ddMap[i] = m_pFilterLogParam->toPlain(dd);
				dd = dd + 0.001;
			}
			*/
			ddMap = new double[1000];
			double dd = 0.001;
			for (int i = 0; i < 1000; i++) {
				ddMap[i] = m_pFilterLogParam->toPlain(dd);
				dd = dd + 0.001;
			}

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

			//doRecalc = new recalcFunc_t[PARTPARAMS];

			void* alignedMemory = _aligned_malloc(PARTPARAMS * sizeof(recalcFunc_t), alignof(recalcFunc_t));
			doRecalc = static_cast<recalcFunc_t*>(alignedMemory);

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
	paramThresh = (10000 / sampleRate);
	syncLFO.pitchShiftFactor = 1;
	/*
	monoVoice.id = -1;//figure
	monoVoice.voiceId = -1;//which


	monoVoice.car = 0.0f;
	monoVoice.dcar = 0.0f;

	monoVoice.generator = genIdx;
	monoVoice.rampingUp = false;
	monoVoice.o = 0.0f;

	monoVoice.voiceEnvLfo[0].env = 0.0f;
	monoVoice.voiceEnvLfo[0].cdec = 0.00f;
	monoVoice.voiceEnvLfo[0].cenv = 0.0f;

	monoVoice.voiceEnvLfo[1].env = 1.0f;
	monoVoice.voiceEnvLfo[1].cdec = 0.99f;
	monoVoice.voiceEnvLfo[1].cenv = 1.0f;

	monoVoice.voiceEnvLfo[2].env = 1.0f;
	monoVoice.voiceEnvLfo[2].cdec = 0.99f;
	monoVoice.voiceEnvLfo[2].cenv = 1.0f;

	monoVoice.voiceEnvLfo[3].env = 1.0f;
	monoVoice.voiceEnvLfo[3].cdec = 0.99f;
	monoVoice.voiceEnvLfo[3].cenv = 1.0f;


	//for wavetables
#ifdef wtOsc
	monoVoice.WTOsc.wTTables = &WTTables;
	monoVoice.WTOsc.m_pCurrentTable = &WTTables.m_dSineTable[0];
#endif
	monoVoice.WTOsc.note = 0;
	monoVoice.WTOsc2.note = 0;
	monoVoice.WTOsc.isModulator = false;
	monoVoice.WTOsc.m_uWaveform = monoVoice.WTOsc.RSINE;
	monoVoice.WTOsc2.m_uWaveform = monoVoice.WTOsc.SINE;
	monoVoice.WTOsc2.isModulator = true;
	monoVoice.WTOsc.setSampleRate(&sampleRateDouble);
	monoVoice.WTOsc2.setSampleRate(&sampleRateDouble);

	monoVoice.WTOsc.m_dOscFo = NULL;

	monoVoice.m_Filter1.setSampleRate(sampleRate);
	monoVoice.m_Filter1.reset();

	for (int a = 0; a < 7; a++) {
		for (int b = 0; b < 2; b++) {//make b2 the global one?
			monoVoice.envBank[a][b] = &monoVoice.voiceEnvLfo[a].cenv;
			monoVoice.voiceEnvLfo[a].setSampleRate(&sampleRateDouble);
		}
	}
	*/

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
			for (int b = 0; b < 2; b++) {//make b2 the global one?
				RVoicesMap[v].envBank[a] = &RVoicesMap[v].voiceEnvLfo[a].cenv;
				RVoicesMap[v].voiceEnvLfo[a].setSampleRate(&sampleRateDouble);
			}
		}





	}

	
	


}






void DXGenerator::noteOn(int32* note, int32* velocity, int32* noteID) {
	
	pushNote = false;
	notesSize = notes.size();

	if (cutNote == true) {
		vl = (*note);
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

		if (pushNote == false) {

			if (notesSize >= POLYCOUNT) {
				float leastFloat = 1.0f;
				int leastNote = 0;
				for (int i = 0; i < notesSize; i++) {
					if (RVoicesMap[notes[i]].voiceEnvLfo[1].cenv < leastFloat) {
						leastFloat = RVoicesMap[notes[i]].voiceEnvLfo[1].cenv;
						leastNote = i;
					}
				}
				retriggerVoice(RVoicesMap[notes[leastNote]], true);// kill voice
				notes.push_back((*note));
				RVoicesMap[vl].noteIdx = notesSize - 1;
				processNoteOn(RVoicesMap[vl], note, velocity);
			}else {
				notes.push_back((*note));
				RVoicesMap[vl].noteIdx = notesSize - 1;
				processNoteOn(RVoicesMap[vl], note, velocity);
			}
		}
		else {
			RVoicesMap[vl].WTOsc.m_dOscFoPrev = RVoicesMap[vl].WTOsc.m_dOscFo;
			RVoicesMap[vl].WTOsc2.m_dOscFoPrev = RVoicesMap[vl].WTOsc2.m_dOscFo;
			RVoicesMap[vl].portoInc = 0.0f;

			RVoicesMap[vl].note = (*note);
			retriggerVoice(RVoicesMap[vl], false);
		}

	}else if (cutNote == false) {
		vl = (*noteID);
		RVoicesMap[vl].note = (*note);
		RVoicesMap[vl].velocity = (*velocity);
		RVoicesMap[vl].vel = (*velocity);
		RVoicesMap[vl].velocity = 0.01 * (100 / (127 / (*velocity)));//10??
		RVoicesMap[vl].noteActive = true;
		RVoicesMap[vl].noteIdx = notesSize - 1;
		notes.push_back((*noteID));
		processNoteOn(RVoicesMap[vl], note, velocity);

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
		voiceObj.voiceEnvLfo[1].envInc = -(1000.0*voiceObj.voiceEnvLfo[1].env) / 21 / sampleRate;
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
	voiceObj.voiceEnvLfo[4].m_dDelayTime = ParamClamp2((*EnvLfoCtrls[4]).lfoDelay, LFO_DELAY_TIME_MIN, 1.0f);
	voiceObj.voiceEnvLfo[4].moduloStart = (*EnvLfoCtrls[4]).phaseStart;
	voiceObj.voiceEnvLfo[4].reset();

	voiceObj.voiceEnvLfo[5].m_uWaveform = (*EnvLfoCtrls[5]).lfoType;
	voiceObj.voiceEnvLfo[5].triCutoff = 0.5;
	voiceObj.voiceEnvLfo[5].m_dPulseWidthControl = 50;
	voiceObj.voiceEnvLfo[5].m_dDelayTime = ParamClamp2((*EnvLfoCtrls[5]).lfoDelay, LFO_DELAY_TIME_MIN, 1.0f);
	voiceObj.voiceEnvLfo[5].moduloStart = (*EnvLfoCtrls[5]).phaseStart;
	voiceObj.voiceEnvLfo[5].reset();

	voiceObj.voiceEnvLfo[6].m_uWaveform = (*EnvLfoCtrls[6]).lfoType;
	voiceObj.voiceEnvLfo[6].triCutoff = 0.5;
	voiceObj.voiceEnvLfo[6].m_dPulseWidthControl = 50;
	voiceObj.voiceEnvLfo[6].m_dDelayTime = ParamClamp2((*EnvLfoCtrls[6]).lfoDelay, LFO_DELAY_TIME_MIN, 1.0f);
	voiceObj.voiceEnvLfo[6].moduloStart = (*EnvLfoCtrls[6]).phaseStart;
	voiceObj.voiceEnvLfo[6].reset();


	newIntEnv = ctrlValues[INT_ENV_SRC + paramStartIdx];//WONT WORK ON both gens?
	newOctEnv = ctrlValues[OCT_ENV_SRC + paramStartIdx];
	newModIntEnv = ctrlValues[MOD_INT_ENV_SRC + paramStartIdx];
	newModOctEnv = ctrlValues[MOD_OCT_ENV_SRC + paramStartIdx];
	voiceObj.WTOsc.pitchShiftFactor = 1.0;
	voiceObj.WTOsc2.pitchShiftFactor = 1.0;
	tuneCEnv = ctrlValues[TUNE_ENV_SRC + paramStartIdx];
	fineCEnv = ctrlValues[FINE_ENV_SRC + paramStartIdx];

	if (linkTune == 1) {
		modTuneCEnv = ctrlValues[TUNE_ENV_SRC + paramStartIdx];
		modTuneEnv = TUNE_ENV;
	}
	else if (linkTune == 0) {
		modTuneCEnv = ctrlValues[MOD_TUNE_ENV_SRC + paramStartIdx];
		modTuneEnv = MOD_TUNE_ENV;
	}

	ogModTuneCEnv = ctrlValues[MOD_TUNE_ENV_SRC + paramStartIdx];
	modFineCEnv = ctrlValues[MOD_FINE_ENV_SRC + paramStartIdx];

	squareEnvSrc = ctrlValues[WIDTH_ENV_SRC + paramStartIdx];
	triEnvSrc = ctrlValues[WIDTH_ENV_SRC + paramStartIdx];
	modSquareEnvSrc = ctrlValues[MOD_WIDTH_ENV_SRC + paramStartIdx];
	modTriEnvSrc = ctrlValues[MOD_WIDTH_ENV_SRC + paramStartIdx];

	panEnvSrc = ctrlValues[PAN_LR_ENV_SRC + paramStartIdx ];

	filterEnvSrc = controls[FILTER_ENV_SRC];//MAKE PER GEN LIKE + PARTPARAMIDX etc
	//filterSrc = static_cast<int>(ceil(filterEnvSrc / (filterEnvSrc + 1)));//can be calced outside?
	filterSrc = static_cast<int>(filterEnvSrc);
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
	int dv;
	if (cutNote == true) {
		dv = (*note);
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
	else if (cutNote == false) {
		dv = (*noteID);

		// need som ekind of note off event here
	}
		//int dv = (*note);
			//this handles the envelopes but not the notes
			//make this per env::
			
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
	//PROCESS RAMPED PARAMS
	for (int i = 0; i < GENRAMPEDPARAMSLENGTH; i++) {
		rampedParams[i].processLevel();
	}
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
		
		int nSize = notes.size();
		//#pragma loop(hint_parallel(4))
		for (int n = 0; n < nSize; n++) {
			//change to functional switch
			
			int i = notes[n];
			
			if (RVoicesMap[i].rampingDown == true) {
				interpolateVoice(RVoicesMap[i]);
				o += RVoicesMap[i].o;
				o1 += RVoicesMap[i].o1;
				o2 += RVoicesMap[i].o2;
			}
			else {
				singleVoiceProcessing(RVoicesMap[i]);
				o += RVoicesMap[i].o;
				o1 += RVoicesMap[i].o1;
				o2 += RVoicesMap[i].o2;
			}
			
			//singleVoiceProcessing(&RVoicesMap[i]);
			//o += RVoicesMap[i].o;

		}
	
	
}



void DXGenerator::singleVoiceProcessing(RVoice& voiceObj) {


	//voiceObj.sampleCount = voiceObj.sampleCount + 1;
	//update all envs
	//find a way to optimize
	voiceObj.voiceEnvLfo[0].env = 0.0;
	voiceObj.voiceEnvLfo[0].cenv = 0.0;

	voiceObj.e = voiceObj.voiceEnvLfo[1].env;
	voiceObj.voiceEnvLfo[1].env = voiceObj.e * voiceObj.voiceEnvLfo[1].cdec;
	voiceObj.voiceEnvLfo[1].cenv += voiceObj.voiceEnvLfo[1].catt * (voiceObj.e - voiceObj.voiceEnvLfo[1].cenv);

	voiceObj.e = voiceObj.voiceEnvLfo[2].env;
	voiceObj.voiceEnvLfo[2].env = voiceObj.e * voiceObj.voiceEnvLfo[2].cdec;
	voiceObj.voiceEnvLfo[2].cenv += voiceObj.voiceEnvLfo[2].catt * (voiceObj.e - voiceObj.voiceEnvLfo[2].cenv);
	
	voiceObj.e = voiceObj.voiceEnvLfo[3].env;
	voiceObj.voiceEnvLfo[3].env = voiceObj.e * voiceObj.voiceEnvLfo[3].cdec;
	voiceObj.voiceEnvLfo[3].cenv += voiceObj.voiceEnvLfo[3].catt * (voiceObj.e - voiceObj.voiceEnvLfo[3].cenv);

	
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
	/*
	float dd = voiceObj.voiceEnvLfo[6].m_dFo;
	float da = voiceObj.voiceEnvLfo[6].m_dFoModLin;
	float db = voiceObj.voiceEnvLfo[6].m_dFoRatio;
	float dc = voiceObj.voiceEnvLfo[6].pitchShiftFactor;
	float dx = voiceObj.voiceEnvLfo[6].m_dSampleRateF;
	*/
	(this->*modChoice[ctrlValues[MOD_TYPE_ + paramStartIdx]])(&voiceObj);

	//see if we can separate th vectorizable vs non vectorizable
	//how many static cast can we get rid of?
	//how many index lookups can we precompute?
	//maybe env bank ones , choose env doesn't need to be real time

	
	voiceObj.newInt = ctrlValues[INT_ + paramStartIdx] + static_cast<int>(controls[INT_ENV] * (*voiceObj.envBank[newIntEnv]) * 13);
	voiceObj.newOct = (ctrlValues[OCT_ + paramStartIdx]) + static_cast<int>((controls[OCT_ENV] * (*voiceObj.envBank[newOctEnv]) * 6));
	if (voiceObj.newOct < 1) {
		voiceObj.newOct = 1;
	}
	if (voiceObj.newOct > 5) {
		voiceObj.newOct = 5;
	}
	voiceObj.newNote = voiceObj.WTOsc.note + (voiceObj.newInt + ((voiceObj.newOct - 3 )* 12.0));
	voiceObj.tuneC = (((controls[TUNE_]) + (controls[TUNE_ENV] * (*voiceObj.envBank[tuneCEnv])))) + (((controls[FINE_] ) + (controls[FINE_ENV] * (*voiceObj.envBank[fineCEnv]))) / 100 );
	voiceObj.WTOsc.m_dOscFo = (midiFreqTable[voiceObj.newNote]) + (((midiFreqTable[voiceObj.newNote]) * 2) *  voiceObj.tuneC);
	voiceObj.WTOsc.m_dOscFo = fmax(OSC_FO_MIN, voiceObj.WTOsc.m_dOscFo);
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
	voiceObj.WTOsc.update();
	voiceObj.WTOsc.triCutoff = ((rampedParams[WIDTH_RAMPINDEX].level) + ((controls[WIDTH_ENV] * widthPole) * (*voiceObj.envBank[triEnvSrc])));
	voiceObj.WTOsc.triCutoff = fmax(fmin(voiceObj.WTOsc.triCutoff, 0.9), 0.1);
	//MODULATOR TUNING
	voiceObj.newModInt = ctrlValues[MOD_INT_ + paramStartIdx] + static_cast<int>(controls[MOD_INT_ENV] * (*voiceObj.envBank[newModIntEnv]) * 13);
	voiceObj.newModOct = (ctrlValues[MOD_OCT_ + paramStartIdx]) + static_cast<int>((controls[MOD_OCT_ENV] * (*voiceObj.envBank[newModOctEnv]) * 13));
	voiceObj.newNote = voiceObj.WTOsc2.note + (voiceObj.newModInt + ((voiceObj.newModOct - 6) * 12.0));
	voiceObj.newNote = ParamClamp2(voiceObj.newNote, 1, 109);
	voiceObj.tuneM = (((controls[MOD_TUNE_]) + (controls[modTuneEnv] * (*voiceObj.envBank[modTuneCEnv])))) + (((controls[MOD_FINE_]) + (controls[MOD_FINE_ENV] * (*voiceObj.envBank[modFineCEnv]))) / 100);

	voiceObj.WTOsc2.m_dOscFo = (midiFreqTable[voiceObj.newNote]) + (((midiFreqTable[voiceObj.newNote]) * 2) *  voiceObj.tuneM);
	voiceObj.WTOsc2.m_dOscFo = fmax(OSC_FO_MIN, voiceObj.WTOsc2.m_dOscFo);

	

	voiceObj.WTOsc2.triCutoff = ((rampedParams[MOD_WIDTH_RAMPINDEX].level) + (((controls[MOD_WIDTH_ENV] * modWidthPole) * (*voiceObj.envBank[modTriEnvSrc]))));
	voiceObj.WTOsc2.triCutoff = fmax(fmin(voiceObj.WTOsc2.triCutoff, 0.9), 0.1);
	
	
	//DO MODULATIONS 
	/*
	(this->*modChoice[ctrlValues[MOD_TYPE_ + paramStartIdx]])(&voiceObj);
	*/
	voiceObj.osc1pure = voiceObj.WTOsc.doOscillate();

	/* make || vectorized */
	voiceObj.gainerTemp = ParamClamp2((rampedParams[GAIN_RAMPINDEX].level) + (controls[GAIN_ENV] * (*voiceObj.envBank[ctrlValues[GAIN_ENV_SRC + paramStartIdx]])), silenceCutoff, 1.0);
	//voiceObj.o = voiceObj.gainerTemp * (voiceObj.voiceEnvLfo[1].cenv * (voiceObj.WTOsc.doOscillate() + (voiceObj.osc2out * (rampedParams[MOD_OSC_GAIN_RAMPINDEX].level))));
	voiceObj.o = voiceObj.gainerTemp * (voiceObj.voiceEnvLfo[1].cenv * (voiceObj.osc1pure + (voiceObj.osc2out * (rampedParams[MOD_OSC_GAIN_RAMPINDEX].level))));
	//voiceObj.o1 = voiceObj.o;
	//voiceObj.o2 = voiceObj.o;
	
		
	//voiceObj.filterEnv = controls[FILTER_ENV] * (*voiceObj.envBank[static_cast<int>(filterEnvSrc)]);
	voiceObj.filterEnv = controls[FILTER_ENV] * (*voiceObj.envBank[filterSrc]);

	//int ddIdx = static_cast<int>((ParamClamp2((rampedParams[FILTER_RAMPINDEX].level) + voiceObj.filterEnv, 0.01, .9999)) * 1000 );
	//voiceObj.m_Filter1.m_dFcControl = ddMap[ddIdx];
	voiceObj.m_Filter1.m_dFcControl = m_pFilterLogParam->toPlain(ParamClamp2((rampedParams[FILTER_RAMPINDEX].level) + voiceObj.filterEnv, 0.01, 1.0));
	//voiceObj.m_Filter1.m_dFcControl = m_pFilterLogParam->toPlain((rampedParams[FILTER_RAMPINDEX].level) + voiceObj.filterEnv);
	//logparamtoplain is praticularly heavy


	voiceObj.m_Filter1.update();
	voiceObj.m_Filter1.doFilter2(voiceObj.o);


	voiceObj.pan = ParamClamp2(((rampedParams[PAN_LR_RAMPINDEX].level) + (rampedParams[PAN_LR_ENV_RAMPINDEX].level * (*voiceObj.envBank[panEnvSrc]))), -1, 1 );
	//voiceObj.pan = voiceObj.pan  * voiceObj.voiceEnvLfo[1].cenv;
	voiceObj.prePan = voiceObj.pan;
	float panDiff = voiceObj.pan - voiceObj.prePan;
	if (std::abs(panDiff) > 0.1) {
		//POTENTIAL DIV BY 0
		voiceObj.prePan = voiceObj.prePan + (paramInc * (panDiff / std::abs(panDiff)));
	}
	else {
		voiceObj.prePan = voiceObj.pan;
	}

	
	voiceObj.o1 = ((voiceObj.prePan + 1.0f) / 2.0f) * voiceObj.o;
	voiceObj.o2 = ((voiceObj.prePan - 1.0f) / -2.0f) * voiceObj.o;
	
	


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
	voiceObj.pan = ParamClamp2(((rampedParams[PAN_LR_RAMPINDEX].level) + (controls[PAN_LR_ENV] * (*voiceObj.envBank[panEnvSrc]))), -1, 1);
	//voiceObj.pan = voiceObj.pan  * voiceObj.voiceEnvLfo[1].cenv;
	voiceObj.prePan = voiceObj.pan;
	float panDiff = voiceObj.pan - voiceObj.prePan;
	if (std::abs(panDiff) > 0.1) {
		//POTENTIAL DIV BY 0
		voiceObj.prePan = voiceObj.prePan + (paramInc * (panDiff / std::abs(panDiff)));//POTENTIAL DIV X 0
	}
	else {
		voiceObj.prePan = voiceObj.pan;
	}


	voiceObj.o1 = ((voiceObj.prePan + 1.0f) / 2.0f) * voiceObj.o;
	voiceObj.o2 = ((voiceObj.prePan - 1.0f) / -2.0f) * voiceObj.o;

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
	if (!doRecalc) {
		return;
	}

	if (paramId < 0 || paramId >= PARTPARAMS) {
		return;
	}
	//(this->*DXGenerator::doRecalc[paramId])();
	(this->*doRecalc[paramId])();
	//Address Sanitizer Error: Use of deallocated memory
}


int DXGenerator::doPhaseMod(RVoice* voiceObj) {
	voiceObj->WTOsc2.update();
	voiceObj->osc2out = voiceObj->WTOsc2.doOscillate();
	//voiceObj->WTOsc.m_dPhaseMod = (fmax(0.0, fmin(1.0, (controls[MOD_GAIN_] + (controls[MOD_GAIN_ENV] * (*voiceObj->envBank[ctrlValues[MOD_GAIN_ENV_SRC + paramStartIdx]][0])))))) * ((voiceObj->WTOsc.m_dOscFo / 50) * voiceObj->WTOsc2.doOscillate());
	voiceObj->WTOsc.m_dPhaseMod = (fmax(0.003, fmin(1.0, ((rampedParams[MOD_GAIN_RAMPINDEX].level) + (controls[MOD_GAIN_ENV] * (*voiceObj->envBank[ctrlValues[MOD_GAIN_ENV_SRC + paramStartIdx]])))))) *  voiceObj->osc2out;
	return 0;
}

int DXGenerator::doFreqMod(RVoice* voiceObj) {
	voiceObj->WTOsc2.update();
	voiceObj->osc2out = voiceObj->WTOsc2.doOscillate();
	voiceObj->WTOsc.m_dFoModLin = (fmax(0.003, fmin(1.0, ((rampedParams[MOD_GAIN_RAMPINDEX].level) + (controls[MOD_GAIN_ENV] * (*voiceObj->envBank[ctrlValues[MOD_GAIN_ENV_SRC + paramStartIdx]]))))) * 4) * (voiceObj->WTOsc.m_dOscFo * voiceObj->osc2out);
	return 0;
}

int DXGenerator::doDetuneMod(RVoice* voiceObj) {
	voiceObj->WTOsc2.m_dOscFo = voiceObj->WTOsc2.m_dOscFo + ((bpm / 60) * ((controls[MOD_BEAT_] + static_cast<int>(controls[MOD_BEAT_ENV] * (*voiceObj->envBank[ctrlValues[MOD_BEAT_ENV_SRC + paramStartIdx]]) * 13)) / 4));
	voiceObj->WTOsc2.update();
	voiceObj->osc2out = voiceObj->WTOsc2.doOscillate();
	voiceObj->WTOsc.m_dFoModLin = (fmax(0.003, fmin(1.0, ((rampedParams[MOD_GAIN_RAMPINDEX].level) + (controls[MOD_GAIN_ENV] * (*voiceObj->envBank[ctrlValues[MOD_GAIN_ENV_SRC + paramStartIdx]]))))) * 8) * (voiceObj->WTOsc.m_dOscFo * voiceObj->osc2out);
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

int DXGenerator::recalcModTune() {
	int pIdx = MOD_TUNE_ + paramStartIdx;
	//controls[MOD_TUNE_] = 2.0 * (*partParams[pIdx]) - 1;//uni to bi?
	controls[MOD_TUNE_] = (*partParams[pIdx]) - 0.5;//uni to bi polar???
	controls[MOD_INT_] = ctrlValues[MOD_INT_ + paramStartIdx] + (controls[MOD_FINE_]) + (controls[MOD_TUNE_]) * 72;
	carPitchMultiplier = pitchShiftMultiplier(controls[MOD_INT_] + pitchBendMod + controls[MOD_OCT_] * 12.0 + semiToneMod + centMod / 100.0);
	return 0;
};


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
	//float divisor;
	//divisor = 8.4f;
	//float level = static_cast<int>(((*partParams[pIdx]) * 100.0) * 1 / divisor);
	//controls[MOD_BEAT_] = level;
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
	//not working now that weve turned this off in controller set param normalized
	if ((*partParams[SYNC_GEN_MOD_TUNE]) > 0.0 ) {
		if (linkTune != 1) {
			linkTune = 1;
			parentProcessor->sendIntAttr(&CMSG_LINK_TUNE_CHAR, "0", 1);
			modTuneCEnv = tuneCEnv;
			ogModTuneEnv = MOD_TUNE_ENV;
			modTuneEnv = TUNE_ENV;

		}
	}
	else {
		if (linkTune != 0) {
			linkTune = 0;
			parentProcessor->sendIntAttr(&CMSG_LINK_TUNE_CHAR, "0", 0);
			modTuneCEnv = ogModTuneCEnv;
			modTuneEnv = MOD_TUNE_ENV;
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
	controls[PAN_LR_ENV] = ((*partParams[pIdx]) * 2) - 1.0;
	rampedParams[PAN_LR_ENV_RAMPINDEX].setLevels2(controls[PAN_LR_ENV], 100, &sampleRate);
	return 0;
}

int DXGenerator::recalcPanLREnvSrc() {
	int pIdx = PAN_LR_ENV_SRC + paramStartIdx;
	controls[PAN_LR_ENV_SRC] = ctrlValues[pIdx];
	

	return 0;
}








void DXGenerator::recalculateRel(const float& sampleFloat, RVoice& voiceObj) {

	
};








void DXGenerator::setActive(){
	lfo0 = 0.0f;
	lfo1 = 1.0f; //reset LFO phase
}


}}}