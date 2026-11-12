/*
 *  mdaDX10Processor.h
 *  mda-vst3
 *
 *  Created by Arne Scheffler on 6/14/08.
 *
 *  mda VST Plug-ins
 *
 *  Copyright (c) 2008 Paul Kellett
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "mdaBaseProcessor.h"
#include <math.h>



namespace Steinberg {
namespace Vst {
namespace rs {
//-----------------------------------------------------------------------------
//class DX10Processor : public BaseProcessor, public Constants
class DX10Processor : public BaseProcessor
{
public:

	

	tresult PLUGIN_API connect(IConnectionPoint* other);

	DX10Processor ();
	~DX10Processor ();

	void unLinkGenParams();
	
	std::wstring valueFromRegistry;
	bool regKeyFound = false;
	bool regKeyLegit = false;

	std::string regErr1;

	//DXGenerator generators[GENCOUNT];

	double procValues[1000];

	std::unordered_map<int32, int32> noteToIDMap;

	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate () SMTG_OVERRIDE;
	tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE;

	tresult PLUGIN_API notify(IMessage* message) SMTG_OVERRIDE;

	void doProcessing (ProcessData& data) SMTG_OVERRIDE;

	void preProcess(ProcessData& data) SMTG_OVERRIDE;
	
	bool hasProgram() const SMTG_OVERRIDE { return true; }


	void sortEventsByOffset(int32 * notes, int32 npos);

	Steinberg::uint32 getCurrentProgram () const SMTG_OVERRIDE { return currentProgram; }
	void setCurrentProgram (Steinberg::uint32 val) SMTG_OVERRIDE;
	void setCurrentProgramNormalized (ParamValue val) SMTG_OVERRIDE;
	virtual bool processParameterChanges(IParameterChanges* changes);

//-----------------------------------------------------------------------------
	static FUnknown* createInstance (void*) { return (IAudioProcessor*)new DX10Processor; }
	static FUID uid;
//-----------------------------------------------------------------------------
	int32 genRecalcIdx = 0;
	int32 genCountIdx = 0;
	static float programParams[20][GPARAMS];
	//float programParams[20][GPARAMS] = { 0.0f };
	float initProgram[GPARAMS] = { 0.0f };
	//static float presetParams[32];
	
	int quantDivisor = 0;

	int loadPID;
	float loadVal;

	Env EnvLfoCtrls[7];
	CLFO mainEnvLfo[7];

	float sampleRate;

	float mainEnv1Rel;
	float mainEnv1ReleaseToggle = 1.0;
	float mainEnv1Att = 0.0;
	float mainEnv1Dec = 0.0;
	float mainEnv1e = 0.0;
	float mainEnv1Env = 0.0;
	float mainEnv1Targ = 0.0;
	float mainEnv1Cenv = 0.0;

	float voiceEnv1Rel= 0.0;
	float voiceEnv1ReleaseToggle = 1.0;
	float voiceEnv1Att = 0.0;
	float voiceEnv1Dec = 0.0;


	bool mainEnv1RampingDown = false;
	bool mainEnv1RampingUp = false;
	float mainEnv1Inc = 0.0;



	float ifs;

	int theval;
	//delete

	ParamRampObj rampedParams[PROCRAMPEDPARAMSLENGTH];
	int MAINENV1_RAMPINDEX = 0;

	double bpm = 120.0;  // B-04: used if the host sends no (valid) tempo
	void dispatchNoteEvent(int32 note, int32 velocity);  // note-on (vel > 0) or note-off (vel == 0)

	float testfl = 0.0;
	
	int (DX10Processor::*recalculators[1000])(double value);

	int recalcSyncAttack(double value);

	int recalcSyncDec(double value);

	int recalcSyncRel(double value);

	int recalcSyncTune(double value);

	int recalcSyncFine(double value);

	int recalcSyncInt(double value);

	int recalcSyncModgain(double value);

	int recalcSyncModOct(double value);

	int recalcSyncModInt(double value);

	int recalcSyncModTune(double value);
	
	int recalcSyncModFine(double value);

	int recalcSyncGain(double value);

	int recalcSyncModBeat(double value);

	int recalcEnv1Attack(double value);
	int recalcEnv1Decay(double value);
	int recalcEnv1Release(double value);
	int recalcEnv1DecayToggle(double value);
	int recalcEnv1Delay(double value);

	int recalcEnv2Attack(double value);
	int recalcEnv2Decay(double value);
	int recalcEnv2Release(double value);
	int recalcEnv2DecayToggle(double value);
	int recalcEnv2Delay(double value);

	int recalcEnv3Attack(double value);
	int recalcEnv3Decay(double value);
	int recalcEnv3Release(double value);
	int recalcEnv3DecayToggle(double value);
	int recalcEnv3Delay(double value);

	int recalcLfo1Rate(double value);
	int recalcLfo1Amp(double value);
	int recalcLfo1Type(double value);
	int recalcLfo1BPM(double value);
	int recalcLfo1Delay(double value);

	int recalcLfo2Rate(double value);
	int recalcLfo2Amp(double value);
	int recalcLfo2Type(double value);
	int recalcLfo2BPM(double value);
	int recalcLfo2Delay(double value);

	int recalcLfo3Rate(double value);
	int recalcLfo3Amp(double value);
	int recalcLfo3Type(double value);
	int recalcLfo3BPM(double value);
	int recalcLfo3Delay(double value);

	int recalcResetWidth(double value);


	int recalcToggleGenView(double value);

	int recalcLfo1Phase(double value);
	int recalcLfo2Phase(double value);
	int recalcLfo3Phase(double value);
	int recalcStepCtrlTarg(double value);
	int recalcStepCtrl1(double value);
	int recalcStepCtrl2(double value);
	int recalcStepCtrl3(double value);
	int recalcStepCtrl4(double value);
	int recalcStepCtrl5(double value);
	int recalcStepCtrl6(double value);
	int recalcStepCtrl7(double value);
	int recalcStepCtrl8(double value);
	int recalcStepCtrl9(double value);
	int recalcStepCtrl10(double value);
	int recalcStepCtrl11(double value);
	int recalcStepCtrl12(double value);
	int recalcPanLR(double value);
	int recalcPanLREnv(double value);
	int recalcPanLREnvSrc(double value);

	int64 STEP_SWITCH_VAL;
	double LFO_RATE_VAL;
	double panEnv = 0.0;
	int panEnvSrc = 0.0;

	
	//overwite necessar recalculator methods to reflect newglobalctrl schema
	//get rid of all of these from here:
	/*
	int recalcMainEnvAttack(double value);
	int recalcMainEnvDecay(double value);
	int recalcMainEnvRelease(double value);

	int recalcVoiceEnvAttack(double value);
	int recalcVoiceEnvDecay(double value);
	int recalcVoiceEnvRelease(double value);
	

	int recalcTOGGLE_MODGAIN_MOD_1_GLOBAL(double value);
	int toggleModGainMod1 = 0;
	int recalcTOGGLE_MODGAIN_MOD_2_GLOBAL(double value);
	int toggleModGainMod2 = 0;
	int recalcTOGGLE_MODGAIN2_MOD_1_GLOBAL(double value);
	int toggleMod2GainMod1 = 0;//should be toggleModgain2Mod1
	int recalcTOGGLE_MODGAIN2_MOD_2_GLOBAL(double value);
	int toggleMod2GainMod2 = 0;//should be toggleModgain2Mod2
	int _TOGGLE_MODPITCH_MOD_1_GLOBAL(double value);
	int toggleModPitchMod1 = 0;
	int _TOGGLE_MODPITCH_MOD_2_GLOBAL(double value);
	int toggleModPitchMod2 = 0;
	int _TOGGLE_MODPITCH2_MOD_1_GLOBAL(double value);
	int toggleModPitch2Mod1 = 0;
	int _TOGGLE_MODPITCH2_MOD_2_GLOBAL(double value);
	int toggleModPitch2Mod2 = 0;
	int _TOGGLE_PITCH_MOD_1_GLOBAL(double value);
	int togglePitchMod1 = 0;
	int _TOGGLE_PITCH_MOD_2_GLOBAL(double value);
	int togglePitchMod2 = 0;
	int _TOGGLE_PITCH2_MOD_1_GLOBAL(double value);
	int togglePitch2Mod1 = 0;
	int _TOGGLE_PITCH2_MOD_2_GLOBAL(double value);
	int togglePitc2hMod2 = 0;
	int _TOGGLE_MAINGAIN_MOD_1_GLOBAL(double value);
	int toggleMainGainMod1 = 0;
	int _TOGGLE_MAINGAIN_MOD_2_GLOBAL(double value);
	int toggleMainGainhMod2 = 0;
	int _TOGGLE_MAINGAIN2_MOD_1_GLOBAL(double value);
	int toggleMmainGain2Mod1 = 0;
	int _TOGGLE_MAINGAIN2_MOD_2_GLOBAL(double value);
	int toggleMainGain2Mod2 = 0;

	int _TOGGLE_PHASE_MOD_1_GLOBAL(double value);
	int togglePhaseMod1 = 0;
	int _TOGGLE_PHASE_MOD_2_GLOBAL(double value);
	int togglePhaseMod2 = 0;
	int _TOGGLE_PHASE2_MOD_1_GLOBAL(double value);
	int togglePhase2Mod1 = 0;
	int _TOGGLE_PHASE2_MOD_2_GLOBAL(double value);
	int togglePhase2Mod2 = 0;
	//get rid of all of these to here:
	*/

	void retriggerMainEnv();
	void processNewMainEnv();
	void mainEnvProcessing();

	int msgInt;

	int32 sampleFrames;
	float* out1;
	float* out2;
	int32 event;
	int32 frame;
	int32 frames;
	int32 note;
	int32 vel;
	int32 silenceCounter = 0;
	int32 quarterSecond = 0;
	int32 noteID = 0;

	int32 v;
	float o;

protected:
	void checkSilence (ProcessData& data) SMTG_OVERRIDE {}
	void setParameter (ParamID index, ParamValue newValue, int32 sampleOffset) SMTG_OVERRIDE;
	void processEvents (IEventList* events) SMTG_OVERRIDE;
	void noteOn(int32 note, int32 velocity, int32 noteID);

	float Fs;

	enum {
		
	};
	int32 notes[EVENTBUFFER + 8];  //list of delta|note|velocity for current block

	
	int32 sustain, activevoices, K;

	

	Steinberg::uint32 currentProgram;


	int32 setParamSetStartIdx;
	
	virtual tresult PLUGIN_API setState(IBStream* state) override;

};

}}} // namespaces
