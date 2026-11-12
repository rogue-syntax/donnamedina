/*
 *  mdaDX10Controller.h
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
#include "NVGViewController.h"
#include "mdaBaseController.h"
#include "mdaDX10Processor.h"//why
#include <string>

namespace Steinberg {
namespace Vst {
namespace rs {



//-----------------------------------------------------------------------------
//class DX10Controller : public BaseController, public Constants
class DX10Controller : public BaseController
{
public:
	DX10Controller ();
	~DX10Controller ();
	//virtual void willClose(VST3Editor* editor) override;
	//IMessage* stringCtrlMsg;
	class ChangeSwitch {
	public:
		virtual int doChangeSwitch(int pid, double value, DX10Controller* ctrlr);
		int x = 0;
	};
	class VertSwitchChange : public ChangeSwitch {
	public:
		virtual int doChangeSwitch(int pid, double value, DX10Controller* ctrlr) override;

	};
	class DoNothingChange : public ChangeSwitch {
	public:
		virtual int doChangeSwitch(int pid, double value, DX10Controller* ctrlr) override;

	};
	DoNothingChange doNothingChange;
	VertSwitchChange vertSwitchChange;
	NVGViewController* nvgViewController = NULL;
	IController* createSubController(const char* name, const IUIDescription* description, VST3Editor* editor) override;
	virtual IPlugView* PLUGIN_API createView(FIDString name);

	void controlWasChanged(int tag, double value);
	
	std::array< int(DX10Controller::*)(int, double), 1000> onCtrlChange;
	std::array< int(DX10Controller::*)(int, double), 1000> onCtrlChange2;
	//int(DX10Controller::*std::array<onCtrlChange[1000])(int pid, double valueDbl);

	virtual void setViewsFromBase() override;


	virtual void doFromBase(int pid, double value);

	String128 stringCtrlTChar;

	String128 modTypeCtrlTChar;
	std::wstring modTypeCtrlWString;
	int modTypeCtrlInt;

	double modTypeNrml = 0.0;
	 
	double valTypeNrml = 0.0;

	int stringCtrlInt;

	std::wstring stringCtrlWString;

	int widthReset(int pid, double valueDbl);

	int onBadReg(int pid, double valueDbl);

	int onStepCtrlButton(int pid, double valueDbl);

	int onStepCtrlTarg(int pid, double valueDbl);

	int onToggleButton(int pid, double valueDbl);

	int onEnvSrc(int pid, double valueDbl);

	int onEnv(int pid, double valueDbl);

	int onEnvBipolar(int pid, double valueDbl);

	int onTuneSwitch(int pid, double valueDbl);

	int onPanEnv(int pid, double valueDbl);

	int onEnvLev(int pid, double valueDbl);

	int onLevel(int pid, double valueDbl);

	int toggleGenView(int pid, double valueDbl);

	int changeVertSwitch(int pid, double valueDbl);

	int changeLFORate(int pid, double valueDbl);

	int onChangeModType(int pid, double valueDbl);

	int onChangeOSCType(int pid, double valueDbl);

	int onChangeLFOOSCType(int pid, double valueDbl);

	int onChangeStepTuner(int pid, double valueDbl);

	int onStepCtrl2(int pid, double valueDbl);
	
	int onResetLevel(int pid, double valueDbl);

	int stepCtrlTargIdx = 0;

	int stepCtrlTargPID;
	
	int lastStepCtrlTargPID = 10;

	int stepCtrlTargSize;

	int selectedEnvSrc;

	int selectedModType;

	std::map<int, int> stepCtrlMap;

	//int vertSwitchPids[_GEN_STEP_PARAM_COUNT_] = { OSC_, WIDTH_ENV_SRC, TUNE_ENV_SRC, FINE_ENV_SRC, OCT_, OCT_ENV_SRC, INT_, INT_ENV_SRC, MOD_OSC_, MOD_TYPE_, MOD_GAIN_ENV_SRC, MOD_WIDTH_ENV_SRC, MOD_TUNE_ENV_SRC, MOD_FINE_ENV_SRC, MOD_OCT_, MOD_OCT_ENV_SRC, MOD_INT_, MOD_INT_ENV_SRC, MOD_BEAT_, MOD_BEAT_ENV_SRC, GAIN_ENV_SRC, FILTER_ENV_SRC, PAN_LR_ENV_SRC };
	
	int doNothing(int pid, double valueDbl);

	Map::hash<char*, int> pidMap;

	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate () SMTG_OVERRIDE;
	//tresult PLUGIN_API notify(IMessage* message) SMTG_OVERRIDE;
	tresult PLUGIN_API setParamNormalized (ParamID tag, ParamValue value) SMTG_OVERRIDE;

	tresult PLUGIN_API getParamStringByValue (ParamID tag, ParamValue valueNormalized, String128 string) SMTG_OVERRIDE;
	
	int safeStoi(std::wstring* Str, size_t *_Idx = nullptr, int _Base = 10);
	
	int stepCtrlTargets[9];
	int stepCtrlTargetSizes[9];



//-----------------------------------------------------------------------------
	static FUnknown* createInstance (void*) { return (IEditController*)new DX10Controller; }
	static FUID uid;

	StringListParameter* strTypeParams[20];

	StringListParameter* strTypeEnvParams[3];

	int testX;
	int testY;
	char* testStr;

	

	/*
DONT FORGET TO ADD PARAM CONT INTS TO CONTROLLER.H (MDADX10CONTROLLER)
SWITCH PARAMS NEED TO BE ADDED TO mdadx10controller vertSwitchPids[i] as well
*/
	
	/*
		//const int refrence, number of steps, 0 steps forknob, 
		int paramProto[PARTPARAMS][2] = {
		{OSC_, 4},//0-4
		{WIDTH_, 0},
		{WIDTH_ENV, 0},
		{WIDTH_ENV_SRC, 7},
		{TUNE_, 0},
		{TUNE_ENV, 0},
		{TUNE_ENV_SRC, 7},
		{FINE_, 0},
		{FINE_ENV, 0},
		{FINE_ENV_SRC, 7},
		{OCT_, 6 },
		{OCT_ENV, 0},
		{OCT_ENV_SRC, 7},//LETS GO 0 - 6
		{INT_, 13 },// BUT 1-12 HERE
		{INT_ENV, 0},
		{INT_ENV_SRC, 7},
		{MOD_OSC_, 4},
		{MOD_TYPE_, 4},
		{MOD_GAIN_, 0},
		{MOD_GAIN_ENV, 0},
		{MOD_GAIN_ENV_SRC, 7},
		{MOD_WIDTH_, 0},
		{MOD_WIDTH_ENV, 0},
		{MOD_WIDTH_ENV_SRC, 7},
		{MOD_TUNE_, 0},
		{MOD_TUNE_ENV, 0 },
		{MOD_TUNE_ENV_SRC, 7},
		{MOD_FINE_, 0},
		{MOD_FINE_ENV, 0},
		{MOD_FINE_ENV_SRC, 7},
		{MOD_OCT_, 13},
		{MOD_OCT_ENV, 0 },
		{MOD_OCT_ENV_SRC, 7},
		{MOD_INT_, 13},
		{MOD_INT_ENV, 0},
		{MOD_INT_ENV_SRC, 7},
		{MOD_BEAT_, 13},
		{MOD_BEAT_ENV, 0 },
		{MOD_BEAT_ENV_SRC, 7},
		{GAIN_, 0 },
		{GAIN_ENV, 0 },
		{GAIN_ENV_SRC, 7 },
		{FILTER_, 0},
		{FILTER_ENV, 0},
		{FILTER_ENV_SRC, 7},
		{FILTER_Q, 0},
		{AUX_10, 0},
		{AUX_11, 0},
		{AUX_12, 0},
		{MOD_OSC_GAIN, 0},
		{SYNC_GEN_MOD_TUNE, 0},
		{SYNC_GEN_MOD_FINE, 0},
		{PAN_LR, 0},
		{PAN_LR_ENV, 0},
		{PAN_LR_ENV_SRC, 7}
		}; 

		//GENERATOR PARAMS, INDEX = TO CONT INT REFRENCE
		char* ParamDef[PARTPARAMS] = {
		"OSC_",
		"WIDTH_",
		"WIDTH_ENV",
		"WIDTH_ENV_SRC",
		"TUNE_",
		"TUNE_ENV",
		"TUNE_ENV_SRC",
		"FINE_",
		"FINE_ENV",
		"FINE_ENV_SRC",
		"OCT_",
		"OCT_ENV",
		"OCT_ENV_SRC",
		"INT_",
		"INT_ENV",
		"INT_ENV_SRC",
		"MOD_OSC_",
		"MOD_TYPE_",
		"MOD_GAIN_",
		"MOD_GAIN_ENV",
		"MOD_GAIN_ENV_SRC",
		"MOD_WIDTH_",
		"MOD_WIDTH_ENV",
		"MOD_WIDTH_ENV_SRC",
		"MOD_TUNE_",
		"MOD_TUNE_ENV",
		"MOD_TUNE_ENV_SRC",
		"MOD_FINE_",
		"MOD_FINE_ENV",
		"MOD_FINE_ENV_SRC",
		"MOD_OCT_",
		"MOD_OCT_ENV",
		"MOD_OCT_ENV_SRC",
		"MOD_INT_",
		"MOD_INT_ENV",
		"MOD_INT_ENV_SRC",
		"MOD_BEAT_",
		"MOD_BEAT_ENV",
		"MOD_BEAT_ENV_SRC",
		"GAIN_",
		"GAIN_ENV",
		"GAIN_ENV_SRC",
		"FILTER_",
		"FILTER_ENV",
		"FILTER_ENV_SRC",
		"FILTER_Q",
		"AUX_10",
		"AUX_11",
		"AUX_12",
		"MOD_OSC_GAIN",
		"SYNC_GEN_MOD_TUNE",
		"SYNC_GEN_MOD_FINE",
		"PAN_LR",
		"PAN_LR_ENV",
		"PAN_LR_ENV_SRC"
		};
	
		//use envLfoSetupNames[ envLfoSetup[i] - envLfoSetup[0] ] to get name
		char* envDef[ENV_PARAMS] = {
		"ENV_1_ATTACK",
		"ENV_1_DECAY",
		"ENV_1_RELEASE",
		"ENV_1__DECAYTOGGLE",
		"ENV_1__DELAY",
		"ENV_2_ATTACK",
		"ENV_2_DECAY",
		"ENV_2_RELEASE",
		"ENV_2__DECAYTOGGLE",
		"ENV_2__DELAY",
		"ENV_3_ATTACK",
		"ENV_3_DECAY",
		"ENV_3_RELEASE",
		"ENV_3__DECAYTOGGLE",
		"ENV_3__DELAY",
		"LFO_1_RATE",
		"LFO_1_AMP",
		"LFO_1_TYPE",
		"LFO_1_BPM",
		"LFO_1_DELAY",
		"LFO_2_RATE",
		"LFO_2_AMP",
		"LFO_2_TYPE",
		"LFO_2_BPM",
		"LFO_2_DELAY",
		"LFO_3_RATE",
		"LFO_3_AMP",
		"LFO_3_TYPE",
		"LFO_3_BPM",
		"LFO_3_DELAY",
		"WIDTH_RESET",
		"TUNE_RESET",
		"FINE_RESET",
		"MOD_GAIN_RESET",
		"MOD_WIDTH_RESET",
		"MOD_TUNE_RESET",
		"MOD_FINE_RESET",
		"TOGGLE_GEN_VIEW",
		"LFO_1_PHASE",
		"LFO_2_PHASE",
		"LFO_3_PHASE",
		"STEP_CTRL_TARG",
		"STEP_CTRL_1",
		"STEP_CTRL_2",
		"STEP_CTRL_3",
		"STEP_CTRL_4",
		"STEP_CTRL_5",
		"STEP_CTRL_6",
		"STEP_CTRL_7",
		"STEP_CTRL_8",
		"STEP_CTRL_9",
		"STEP_CTRL_10",
		"STEP_CTRL_11",
		"STEP_CTRL_12"
		};



		float envProto[ENV_PARAMS][3] = {
			{(float)ENV_1_ATTACK, 0, 0.25},
			{(float)ENV_1_DECAY, 0, 0.85},
			{(float)ENV_1_RELEASE, 0, 0.85},
			{(float)ENV_1_DECAYTOGGLE, 0,1.0},
			{(float)ENV_1_DELAY, 0, 0.0},
			{(float)ENV_2_ATTACK, 0, 0.25},
			{(float)ENV_2_DECAY, 0, 0.85},
			{(float)ENV_2_RELEASE, 0, 0.85},
			{(float)ENV_2_DECAYTOGGLE, 0, 1.0},
			{(float)ENV_2_DELAY, 0, 0.0},
			{(float)ENV_3_ATTACK, 0, 0.25},
			{(float)ENV_3_DECAY, 0, 0.85},
			{(float)ENV_3_RELEASE, 0, 0.85},
			{(float)ENV_3_DECAYTOGGLE, 0, 1.0},
			{(float)ENV_3_DELAY, 0, 0.0},
			{(float)LFO_1_RATE, 0, 0.25},
			{(float)LFO_1_AMP, 0, 0.25},
			{(float)LFO_1_TYPE, 4, 0.0},
			{(float)LFO_1_BPM, 0, 120.0},
			{(float)LFO_1_DELAY, 0, 0.0},
			{(float)LFO_2_RATE, 0, 0.25},
			{(float)LFO_2_AMP, 0, 0.85},
			{(float)LFO_2_TYPE, 4, 0.85},
			{(float)LFO_2_BPM, 0, 120.0},
			{(float)LFO_2_DELAY, 0, 0.0},
			{(float)LFO_3_RATE, 0, 0.25},
			{(float)LFO_3_AMP, 0, 0.25},
			{(float)LFO_3_TYPE, 4, 0.0},
			{(float)LFO_3_BPM, 0, 120.0},
			{(float)LFO_3_DELAY, 0, 0.0},
			{(float)WIDTH_RESET, 0, 0.0},
			{(float)TUNE_RESET, 0, 0.0},
			{(float)FINE_RESET, 0, 0.0},
			{(float)MOD_GAIN_RESET, 0, 0.0},
			{(float)MOD_WIDTH_RESET, 0, 0.0},
			{(float)MOD_TUNE_RESET, 0, 0.0},
			{(float)MOD_FINE_RESET, 0, 0.0},
			{(float)TOGGLE_GEN_VIEW, 0, 0.0},
			{(float)LFO_1_PHASE, 0, 0.0},
			{(float)LFO_2_PHASE, 0, 0.0},
			{(float)LFO_3_PHASE, 0, 0.0},
			{(float)STEP_CTRL_TARG, 8, 0.0},
			{(float)STEP_CTRL_1, 0, 0.0},
			{(float)STEP_CTRL_2, 0, 0.0},
			{(float)STEP_CTRL_3, 0, 0.0},
			{(float)STEP_CTRL_4, 0, 0.0},
			{(float)STEP_CTRL_5, 0, 0.0},
			{(float)STEP_CTRL_6, 0, 0.0},
			{(float)STEP_CTRL_7, 0, 0.0},
			{(float)STEP_CTRL_8, 0, 0.0},
			{(float)STEP_CTRL_9, 0, 0.0},
			{(float)STEP_CTRL_10, 0, 0.0},
			{(float)STEP_CTRL_11, 0, 0.0},
			{(float)STEP_CTRL_12, 0, 0.0}
			
		};

		int envPID[ENV_PARAMS] = {
			ENV_1_ATTACK,
			ENV_1_DECAY,
			ENV_1_RELEASE,
			ENV_1_DECAYTOGGLE,
			ENV_1_DELAY,
			ENV_2_ATTACK,
			ENV_2_DECAY,
			ENV_2_RELEASE,
			ENV_2_DECAYTOGGLE,
			ENV_2_DELAY,
			ENV_3_ATTACK,
			ENV_3_DECAY,
			ENV_3_RELEASE,
			ENV_3_DECAYTOGGLE,
			ENV_3_DELAY,
			LFO_1_RATE,
			LFO_1_AMP,
			LFO_1_TYPE,
			LFO_1_BPM,
			LFO_1_DELAY,
			LFO_2_RATE,
			LFO_2_AMP,
			LFO_2_TYPE,
			LFO_2_BPM,
			LFO_2_DELAY,
			LFO_3_RATE,
			LFO_3_AMP,
			LFO_3_TYPE,
			LFO_3_BPM,
			LFO_3_DELAY,
			WIDTH_RESET,
			TUNE_RESET,
			FINE_RESET,
			MOD_GAIN_RESET,
			MOD_WIDTH_RESET,
			MOD_TUNE_RESET,
			MOD_FINE_RESET,
			TOGGLE_GEN_VIEW,
			STEP_CTRL_TARG,
			LFO_1_PHASE,
			LFO_2_PHASE,
			LFO_3_PHASE,
			STEP_CTRL_1,
			STEP_CTRL_2,
			STEP_CTRL_3,
			STEP_CTRL_4,
			STEP_CTRL_5,
			STEP_CTRL_6,
			STEP_CTRL_7,
			STEP_CTRL_8,
			STEP_CTRL_9,
			STEP_CTRL_10,
			STEP_CTRL_11,
			STEP_CTRL_12
		};

		
		*/

};

}}} // namespaces
