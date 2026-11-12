/*
 *  mdaDX10Controller.cpp
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
 //#include "constants.h"

#include "mdaDX10Controller.h"




namespace Steinberg {
namespace Vst {
namespace rs {

	LogScale<ParamValue> filterLogScale(MIN_UNIPOLAR,	/* VST GUI Variable MIN (0) */
		MAX_UNIPOLAR,	/* VST GUI Variable MAX (1) */
		MIN_FILTER_FC,	/* filter fc LOW */
		MAX_FILTER_FC,	/* filter fc HIGH */
		FILTER_RAW_MAP,/* the value at position 0.5 will be: */
		FILTER_COOKED_MAP);/* 1800 Hz */

//-----------------------------------------------------------------------------
static const char* presetNames[] = 
{
	"Softly",
	"Prima Donna",
	"Perturb",
	"Polycrystalline",
	"Skillsaw",
	"Brooding Sky Arp",
	"Brooding Sea Strings",

	NULL
};

//-----------------------------------------------------------------------------
FUID DX10Controller::uid (0xD304372E, 0x393E4A29, 0xB6D72C58, 0xD268FD90 );



int DX10Controller::VertSwitchChange::doChangeSwitch(int pid, double value, DX10Controller* ctrlr){
	//changeVertSwitch(pid, value);
	return 0;

};

int DX10Controller::DoNothingChange::doChangeSwitch(int pid, double value, DX10Controller* ctrlr){

	return 0;
};

//void DX10Controller::willClose(VST3Editor* editor) {
	//editor->forget();
	
//}
//MAKE OUR VIEWS

IController* DX10Controller::createSubController(const char* name, const IUIDescription* description, VST3Editor* editor) {
	if (strcmp(name, "nvgViewController") == 0  ) {
		ParameterContainer& pcr = getParameters();

		//nvgViewController = new NVGViewController(editor, pcr);
		//std::unique_ptr<NVGViewController>nvgViewController(new NVGViewController(editor));
		//nvgViewController = std::unique_ptr<NVGViewController>(new NVGViewController(editor));

		
		nvgViewController = new NVGViewController(editor);
		nvgViewController->nvgView->baseController = this;

		//nvgViewController->dx10 = this;
		//NVGViewController nvgViewController(editor);
		//nvgViewController->nvgView->baseController = this;

		nvgViewController->nvgView->lfo1Rate = &lfo4Rate;
		nvgViewController->nvgView->lfo2Rate = &lfo5Rate;
		nvgViewController->nvgView->lfo3Rate = &lfo6Rate;

		

		for (int i = 0; i < PARTPARAMS; i++) {
			nvgViewController->nvgView->NVGParamDef[i] = ParamDef[i];
		}
		if (constants::isRegLegit == true) {
			nvgViewController->nvgView->badReg = false;
		}
		else {
			nvgViewController->nvgView->badReg = true;
		}

		modTypeNrml = EditController::getParamNormalized(MOD_TYPE_);
		EditController::getParamStringByValue(MOD_TYPE_, modTypeNrml, modTypeCtrlTChar);//is there  beeter way to get string than like 15 lines of code?..
		modTypeCtrlWString = modTypeCtrlTChar;
		modTypeCtrlInt = safeStoi(&modTypeCtrlWString);
		if (nvgViewController) {
			nvgViewController->nvgView->onChangeModType(MOD_TYPE_, modTypeCtrlInt);
		}

		/*
		modTypeNrml = EditController::getParamNormalized(LFO_1_BPM);
		int stater = static_cast<int>(modTypeNrml);
		if (nvgViewController) {
			nvgViewController->nvgView->onOnOffBtn(LFO_1_BPM, stater);
			nvgViewController->nvgView->onToggleButton(LFO_1_BPM);
		}

		modTypeNrml = EditController::getParamNormalized(LFO_2_BPM);
		stater = static_cast<int>(modTypeNrml);
		if (nvgViewController) {
			nvgViewController->nvgView->onOnOffBtn(LFO_2_BPM, stater);
			nvgViewController->nvgView->onToggleButton(LFO_2_BPM);
		}

		modTypeNrml = EditController::getParamNormalized(LFO_3_BPM);
		stater = static_cast<int>(modTypeNrml);
		if (nvgViewController) {
			nvgViewController->nvgView->onOnOffBtn(LFO_3_BPM, stater);
			nvgViewController->nvgView->onToggleButton(LFO_3_BPM);
		}

		modTypeNrml = EditController::getParamNormalized(SYNC_GEN_MOD_TUNE);
		stater = static_cast<int>(modTypeNrml);
		if (nvgViewController) {
			nvgViewController->nvgView->onOnOffBtn(SYNC_GEN_MOD_TUNE, stater);
			nvgViewController->nvgView->onToggleButton(SYNC_GEN_MOD_TUNE);
		}
		*/

		return nvgViewController;

	}
	else {
		return 0;
	}
}

void DX10Controller::setViewsFromBase() {

	valTypeNrml = EditController::getParamNormalized(LFO_1_BPM);
	int stater = static_cast<int>(valTypeNrml);
	if (nvgViewController) {
		nvgViewController->nvgView->onOnOffBtn(LFO_1_BPM, stater);
		nvgViewController->nvgView->onToggleButton(LFO_1_BPM);
	}

	valTypeNrml = EditController::getParamNormalized(LFO_2_BPM);
	stater = static_cast<int>(valTypeNrml);
	if (nvgViewController) {
		nvgViewController->nvgView->onOnOffBtn(LFO_2_BPM, stater);
		nvgViewController->nvgView->onToggleButton(LFO_2_BPM);
	}

	valTypeNrml = EditController::getParamNormalized(LFO_3_BPM);
	stater = static_cast<int>(valTypeNrml);
	if (nvgViewController) {
		nvgViewController->nvgView->onOnOffBtn(LFO_3_BPM, stater);
		nvgViewController->nvgView->onToggleButton(LFO_3_BPM);
	}

	valTypeNrml = EditController::getParamNormalized(SYNC_GEN_MOD_TUNE);
	stater = static_cast<int>(valTypeNrml);
	if (nvgViewController) {
		nvgViewController->nvgView->onOnOffBtn(SYNC_GEN_MOD_TUNE, stater);
		nvgViewController->nvgView->onToggleButton(SYNC_GEN_MOD_TUNE);
	}

}

//MAKE OUR EDITOR
IPlugView* PLUGIN_API DX10Controller::createView(FIDString _name) {
	ConstString name(_name);
	if (name == ViewType::kEditor) {
		// --- create the editor using the .uidesc file (in our Resources, edited with WYSIWYG editor in client)
		//
		CRect viewRect(0, 0, 1148, 542);
		rsView = new RSView(this, "Editor", "vstgui.uidesc", viewRect);
		return rsView;
	}
	return 0;
}
//-----------------------------------------------------------------------------
DX10Controller::DX10Controller ()
{
	addBypassParameter = false;

	for (int i = 0; i < 1000; i++) {
		onCtrlChange[i] = &DX10Controller::doNothing;
		onCtrlChange2[i] = &DX10Controller::doNothing;
	}
	for (int i = 0; i < _GEN_STEP_PARAM_COUNT_; i++) {
		onCtrlChange[vertSwitchPids[i]] = &DX10Controller::changeVertSwitch;
		onCtrlChange2[vertSwitchPids[i]] = &DX10Controller::changeVertSwitch;
		//onCtrlChange[(vertSwitchPids[i] + PARTPARAMS)] = &DX10Controller::changeVertSwitch;
	}
	//onCtrlChange[MOD_BEAT_] = &DX10Controller::changeVertSwitch;

	onCtrlChange[OCT_] = &DX10Controller::onTuneSwitch;
	onCtrlChange[INT_] = &DX10Controller::onTuneSwitch;
	onCtrlChange[MOD_OCT_] = &DX10Controller::onTuneSwitch;
	onCtrlChange[MOD_INT_] = &DX10Controller::onTuneSwitch;
	onCtrlChange[MOD_BEAT_] = &DX10Controller::onTuneSwitch;

	onCtrlChange[WIDTH_ENV] = &DX10Controller::onEnv;
	onCtrlChange[TUNE_ENV] = &DX10Controller::onEnv;
	onCtrlChange[FINE_ENV] = &DX10Controller::onEnv;
	onCtrlChange[OCT_ENV] = &DX10Controller::onEnv;
	onCtrlChange[INT_ENV] = &DX10Controller::onEnv;
	onCtrlChange[MOD_GAIN_ENV] = &DX10Controller::onEnv;
	onCtrlChange[MOD_WIDTH_ENV] = &DX10Controller::onEnv;
	onCtrlChange[MOD_TUNE_ENV] = &DX10Controller::onEnv;
	onCtrlChange[MOD_FINE_ENV] = &DX10Controller::onEnv;
	onCtrlChange[MOD_OCT_ENV] = &DX10Controller::onEnv;
	onCtrlChange[MOD_INT_ENV] = &DX10Controller::onEnv;
	onCtrlChange[MOD_BEAT_ENV] = &DX10Controller::onEnv;
	onCtrlChange[GAIN_ENV] = &DX10Controller::onEnv;
	onCtrlChange[FILTER_ENV] = &DX10Controller::onEnv;
	onCtrlChange[PAN_LR_ENV] = &DX10Controller::onEnv;
	onCtrlChange[PAN_LR] = &DX10Controller::onEnv;

	onCtrlChange[WIDTH_] = &DX10Controller::onEnv;
	onCtrlChange[TUNE_] = &DX10Controller::onEnv;
	onCtrlChange[FINE_] = &DX10Controller::onEnv;
	onCtrlChange[MOD_WIDTH_] = &DX10Controller::onEnv;
	onCtrlChange[MOD_TUNE_] = &DX10Controller::onEnv;
	onCtrlChange[MOD_FINE_] = &DX10Controller::onEnv;

	onCtrlChange[MOD_OSC_GAIN] = &DX10Controller::onPanEnv;
	onCtrlChange[GAIN_] = &DX10Controller::onPanEnv;
	onCtrlChange[FILTER_] = &DX10Controller::onPanEnv;
	onCtrlChange[FILTER_Q] = &DX10Controller::onPanEnv;
	onCtrlChange[MOD_GAIN_] = &DX10Controller::onPanEnv;

	onCtrlChange[ENV_1_ATTACK] = &DX10Controller::onEnvLev;
	onCtrlChange[ENV_1_DECAY] = &DX10Controller::onEnvLev;
	onCtrlChange[ENV_1_RELEASE] = &DX10Controller::onEnvLev;
	onCtrlChange[ENV_2_ATTACK] = &DX10Controller::onEnvLev;
	onCtrlChange[ENV_2_DECAY] = &DX10Controller::onEnvLev;
	onCtrlChange[ENV_2_RELEASE] = &DX10Controller::onEnvLev;
	onCtrlChange[ENV_3_ATTACK] = &DX10Controller::onEnvLev;
	onCtrlChange[ENV_3_DECAY] = &DX10Controller::onEnvLev;
	onCtrlChange[ENV_3_RELEASE] = &DX10Controller::onEnvLev;

	onCtrlChange[LFO_1_RATE] = &DX10Controller::onEnvLev;
	onCtrlChange[LFO_1_AMP] = &DX10Controller::onEnvLev;
	onCtrlChange[LFO_1_TYPE] = &DX10Controller::onChangeLFOOSCType;
	onCtrlChange[LFO_2_RATE] = &DX10Controller::onEnvLev;
	onCtrlChange[LFO_2_AMP] = &DX10Controller::onEnvLev;
	onCtrlChange[LFO_2_TYPE] = &DX10Controller::onChangeLFOOSCType;
	onCtrlChange[LFO_3_RATE] = &DX10Controller::onEnvLev;
	onCtrlChange[LFO_3_AMP] = &DX10Controller::onEnvLev;
	onCtrlChange[LFO_3_TYPE] = &DX10Controller::onChangeLFOOSCType;

	onCtrlChange[LFO_1_PHASE] = &DX10Controller::onEnvBipolar;
	onCtrlChange[LFO_1_DELAY] = &DX10Controller::onEnvBipolar;
	onCtrlChange[LFO_2_PHASE] = &DX10Controller::onEnvBipolar;
	onCtrlChange[LFO_2_DELAY] = &DX10Controller::onEnvBipolar;
	onCtrlChange[LFO_3_PHASE] = &DX10Controller::onEnvBipolar;
	onCtrlChange[LFO_3_DELAY] = &DX10Controller::onEnvBipolar;

	onCtrlChange[WIDTH_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[TUNE_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[FINE_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[OCT_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[INT_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[MOD_GAIN_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[MOD_WIDTH_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[MOD_TUNE_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[MOD_FINE_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[MOD_OCT_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[MOD_INT_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[MOD_BEAT_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[GAIN_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[FILTER_ENV_SRC] = &DX10Controller::onEnvSrc;
	onCtrlChange[PAN_LR_ENV_SRC] = &DX10Controller::onEnvSrc;

	onCtrlChange[LFO_1_BPM] = &DX10Controller::onToggleButton;
	onCtrlChange[LFO_2_BPM] = &DX10Controller::onToggleButton;
	onCtrlChange[LFO_3_BPM] = &DX10Controller::onToggleButton;

	onCtrlChange[SYNC_GEN_MOD_TUNE] = &DX10Controller::onToggleButton;
	onCtrlChange[ENV_1_DECAYTOGGLE] = &DX10Controller::onToggleButton;
	onCtrlChange[ENV_2_DECAYTOGGLE] = &DX10Controller::onToggleButton;
	onCtrlChange[ENV_3_DECAYTOGGLE] = &DX10Controller::onToggleButton;

	onCtrlChange[WIDTH_RESET] = &DX10Controller::widthReset;
	onCtrlChange[TUNE_RESET] = &DX10Controller::widthReset;
	onCtrlChange[FINE_RESET] = &DX10Controller::widthReset;
	onCtrlChange[MOD_GAIN_RESET] = &DX10Controller::doNothing;
	onCtrlChange[MOD_WIDTH_RESET] = &DX10Controller::widthReset;
	onCtrlChange[MOD_TUNE_RESET] = &DX10Controller::widthReset;
	onCtrlChange[MOD_FINE_RESET] = &DX10Controller::widthReset;

	onCtrlChange[TOGGLE_GEN_VIEW] = &DX10Controller::doNothing;

	onCtrlChange[MOD_TYPE_] = &DX10Controller::onChangeModType;

	onCtrlChange[OSC_] = &DX10Controller::onChangeOSCType;
	onCtrlChange[MOD_OSC_] = &DX10Controller::onChangeOSCType;



	onCtrlChange[STEP_CTRL_1] = &DX10Controller::onStepCtrlButton;
	onCtrlChange[STEP_CTRL_2] = &DX10Controller::onStepCtrlButton;
	onCtrlChange[STEP_CTRL_3] = &DX10Controller::onStepCtrlButton;
	onCtrlChange[STEP_CTRL_4] = &DX10Controller::onStepCtrlButton;
	onCtrlChange[STEP_CTRL_5] = &DX10Controller::onStepCtrlButton;
	onCtrlChange[STEP_CTRL_6] = &DX10Controller::onStepCtrlButton;
	onCtrlChange[STEP_CTRL_7] = &DX10Controller::onStepCtrlButton;
	onCtrlChange[STEP_CTRL_8] = &DX10Controller::onStepCtrlButton;
	onCtrlChange[STEP_CTRL_9] = &DX10Controller::onStepCtrlButton;
	onCtrlChange[STEP_CTRL_10] = &DX10Controller::onStepCtrlButton;
	onCtrlChange[STEP_CTRL_11] = &DX10Controller::onStepCtrlButton;
	onCtrlChange[STEP_CTRL_12] = &DX10Controller::onStepCtrlButton;

	onCtrlChange[STEP_CTRL_TARG] = &DX10Controller::onStepCtrlTarg;

	onCtrlChange[999] = &DX10Controller::onBadReg;


	stepCtrlTargets[0] = OCT_; stepCtrlTargetSizes[0] = 6;
	stepCtrlTargets[1] = INT_; stepCtrlTargetSizes[1] = 13;
	stepCtrlTargets[2] = MOD_OCT_; stepCtrlTargetSizes[2] = 13;
	stepCtrlTargets[3] = MOD_INT_; stepCtrlTargetSizes[3] = 13;
	stepCtrlTargets[4] = MOD_BEAT_; stepCtrlTargetSizes[4] = 13;
	stepCtrlTargets[5] = LFO_1_RATE; stepCtrlTargetSizes[5] = 11;
	stepCtrlTargets[6] = LFO_2_RATE; stepCtrlTargetSizes[6] = 11;
	stepCtrlTargets[7] = LFO_3_RATE; stepCtrlTargetSizes[7] = 11;
	stepCtrlTargets[8] = LFO_3_RATE; stepCtrlTargetSizes[8] = 11;

	stepCtrlMap[STEP_CTRL_1] = 1;
	stepCtrlMap[STEP_CTRL_2] = 2;
	stepCtrlMap[STEP_CTRL_3] = 3;
	stepCtrlMap[STEP_CTRL_4] = 4;
	stepCtrlMap[STEP_CTRL_5] = 5;
	stepCtrlMap[STEP_CTRL_6] = 6;
	stepCtrlMap[STEP_CTRL_7] = 7;
	stepCtrlMap[STEP_CTRL_8] = 8;
	stepCtrlMap[STEP_CTRL_9] = 9;
	stepCtrlMap[STEP_CTRL_10] = 10;
	stepCtrlMap[STEP_CTRL_11] = 11;
	stepCtrlMap[STEP_CTRL_12] = 12;

	stepCtrlTargPID = stepCtrlTargets[0];
	stepCtrlTargSize = stepCtrlTargetSizes[0];




}


int DX10Controller::onBadReg(int pid, double msg) {
	nvgViewController->nvgView->onBadReg(pid, 1.0);
	return 0;
}



int DX10Controller::widthReset(int pid, double msg) {
	//do nothing
	nvgViewController->nvgView->onResetButton(pid);
	return 0;
}


int DX10Controller::onEnvBipolar(int pid, double msg) {
	msg = (msg - 0.5) * 2;
	std::string Y = std::to_string(msg);
	if (nvgViewController) {
		nvgViewController->nvgView->onEnvBipolar(pid, Y);
	}
	return 0;
}

int DX10Controller::onEnv(int pid, double msg) {
	msg = (msg - 0.5)*2;
	std::string Y = std::to_string(msg);
	if (nvgViewController) {
		nvgViewController->nvgView->onEnv(pid, Y);
	}
	return 0;
}

int DX10Controller::onPanEnv(int pid, double msg) {
	std::string Y = std::to_string(msg);
	if (nvgViewController) {
		nvgViewController->nvgView->onPanEnv(pid, Y);
	}
	return 0;
}

int DX10Controller::onEnvLev(int pid, double msg) {
	std::string Y = std::to_string(msg);
	if (nvgViewController) {
		nvgViewController->nvgView->onEnvLev(pid, Y);
	}
	return 0;
}

int DX10Controller::onEnvSrc(int pid, double msg) {
	selectedEnvSrc = static_cast<int>((msg * 100.0) * 1 / 14.35);
	if ( nvgViewController ) {
		nvgViewController->nvgView->onEnvSrc(pid, selectedEnvSrc);
		//changeVertSwitch(pid, msg);
		//call changeVertSwitch bc we're overwring ctrl event handler target for changing processer params where this was called
	}
	return 0;
}

int DX10Controller::onTuneSwitch(int pid, double msg) {
	float divisor;
	if (pid == OCT_) {
		divisor = 20.99f;
	}
	else {
		divisor = 8.4f;
	}
	selectedEnvSrc = static_cast<int>((msg * 100.0) * 1 / divisor);
	std::string tuneInterval = std::to_string(selectedEnvSrc);
	if (nvgViewController) {
		nvgViewController->nvgView->onTuneSwitch(pid, tuneInterval);
		//changeVertSwitch(pid, msg);
	}
	return 0;
}

int DX10Controller::onChangeOSCType(int pid, double msg) {
	selectedModType = static_cast<int>((msg * 100.0) * 1 / 25);
	if (nvgViewController) {
		nvgViewController->nvgView->onChangeOSCType(pid, selectedModType);
		//changeVertSwitch(pid, msg);
	}
	return 0;
}

int DX10Controller::onChangeLFOOSCType(int pid, double msg) {
	selectedModType = static_cast<int>((msg * 100.0) * 1 / 25);
	if (nvgViewController) {
		nvgViewController->nvgView->onChangeLFOOSCType(pid, selectedModType);
		//changeVertSwitch(pid, msg);
	}
	return 0;
}

int DX10Controller::onChangeModType(int pid, double msg) {
	selectedModType = static_cast<int>(((msg * 100.0) * 1 / 25)-0.01);
	if (nvgViewController) {
		nvgViewController->nvgView->onChangeModType(pid, selectedModType);
		//changeVertSwitch(pid, msg);
	}
	return 0;
}

int DX10Controller::onStepCtrlTarg(int pid, double msg) {
	//stepCtrlTargIdx = static_cast<int>(div((msg * 100), 12).quot);
	stepCtrlTargIdx = static_cast<int>((msg * 100.0)* 1 / 12.5);
	stepCtrlTargPID = stepCtrlTargets[stepCtrlTargIdx];
	stepCtrlTargSize = stepCtrlTargetSizes[stepCtrlTargIdx];
	//this is firing on mousedown vs just mouseup
	if (stepCtrlTargPID != lastStepCtrlTargPID) {
		nvgViewController->nvgView->onStepCtrlSwitch(stepCtrlTargPID);
	}
	//somehow this became poiintee at OSC???
	lastStepCtrlTargPID = stepCtrlTargPID;
	return 0;
}

int DX10Controller::onStepCtrlButton(int pid, double msg) {
	if (msg > 0.0) {
		int stepCtrlMsgInt = stepCtrlMap.at(pid);
		if (stepCtrlMsgInt > stepCtrlTargSize) {
			stepCtrlMsgInt = stepCtrlTargSize;
		}
		float stepCtrlMsgFloat = (1.0f / static_cast<float>(stepCtrlTargSize)) * static_cast<float>(stepCtrlMsgInt);
		//float stepCtrlMsgFloat = static_cast<float>(stepCtrlMsgInt);
		if (stepCtrlTargPID > 100) {
			changeLFORate(stepCtrlTargPID, static_cast<double>(stepCtrlMsgFloat));
		}
		//setParamNormalized(stepCtrlTargPID, stepCtrlMsgFloat);
		performEdit(stepCtrlTargPID, stepCtrlMsgFloat);
		nvgViewController->nvgView->onStepCtrlButton(pid);

	}
	
	return 0;
}

int DX10Controller::onToggleButton(int pid, double msg) {
	int stater = static_cast<int>(msg);
	if (nvgViewController) {		
		nvgViewController->nvgView->onOnOffBtn(pid, stater);
		nvgViewController->nvgView->onToggleButton(pid);
	}
	return 0;
}


int DX10Controller::toggleGenView(int pid, double msg) {
	/*
	if (nvgViewController->nvgView->controlMap[pid]->getValueNormalized() < 1.0) {

		for (int i = OSC_; i <= FILTER_ENV_SRC; i++) {//0 thru 44, missing filter q at 47
				int tempTag = nvgViewController->nvgView->controlMap[i]->getTag();
				if (tempTag >= PARTPARAMS) {
					tempTag = tempTag - PARTPARAMS;
					nvgViewController->nvgView->controlMap[i]->setTag(tempTag);
				}
		}
		//OUT OF SEQUENCE PARAMS
		//filterQ 47
		int tempTag = nvgViewController->nvgView->controlMap[FILTER_Q]->getTag();
		if (tempTag >= PARTPARAMS) {
			tempTag = tempTag - PARTPARAMS;
			nvgViewController->nvgView->controlMap[FILTER_Q]->setTag(tempTag);
		}

		
	}
	else {
		for (int i = OSC_; i <= FILTER_ENV_SRC; i++) {
			int tempTag = nvgViewController->nvgView->controlMap[i]->getTag();
			if (tempTag < PARTPARAMS) {
				tempTag = tempTag + PARTPARAMS;
				nvgViewController->nvgView->controlMap[i]->setTag(tempTag);
			}	
		}
		//OUT OF SEQUENCE PARAMS
		//filterQ 47
		int tempTag = nvgViewController->nvgView->controlMap[FILTER_Q]->getTag();
		if (tempTag < PARTPARAMS) {
			tempTag = tempTag + PARTPARAMS;
			nvgViewController->nvgView->controlMap[FILTER_Q]->setTag(tempTag);
		}

	}
	*/
	return 0;
}
int DX10Controller::ChangeSwitch::doChangeSwitch(int pid, double msg, DX10Controller* ctrlr) {
	//do nothing
	return 0;
}


int DX10Controller::doNothing(int pid, double msg) {
	//do nothing
	return 0;
}

//-----------------------------------------------------------------------------
DX10Controller::~DX10Controller ()
{
}


void DX10Controller::doFromBase(int pid, double value) {
	//int PID = pid;
	//double VAL = value;
	if (nvgViewController) {
		if (nvgViewController->isCreated == true) {
			(this->*onCtrlChange.at(pid))(pid, value);
		}
	}
	

}



//-----------------------------------------------------------------------------
tresult PLUGIN_API DX10Controller::initialize (FUnknown* context)
{

	//CDrawContext* pContext = new CDrawContext();
	

	tresult res = BaseController::initialize (context);
	if (res == kResultTrue)
	{
		IndexedParameter* presetParam = new IndexedParameter (
		    USTRING ("Factory Presets"), 0, DX10Processor::kNumPrograms - 1, 0,
		    ParameterInfo::kCanAutomate | ParameterInfo::kIsProgramChange | ParameterInfo::kIsList,
		    kPresetParam);
		for (int32 i = 0; i < DX10Processor::kNumPrograms; i++) {
			presetParam->setIndexString(i, UString128(presetNames[i]));
			
		}
			
		parameters.addParameter (presetParam);

		

		
		//update with new paramaters /pids 
		//update constans/partparams etc..
		//update all refrences in processing code
		int strParamsCount = 0;
		int strEnvParamsCount = 0;
		ParamID pid = 1000; 
		ParamID envPid = 0;
		//per generator
		//g1
		for (int i = 0; i < GENCOUNT; i++) {
			//IDX+(PARTPARAMS*i)
			for (int x = 0; x < PARTPARAMS; x++) {
				int pidIdx = x + (PARTPARAMS*i);
				std::string namer = ParamDef[paramProto[x][0]];
				if (i == 0) {
					namer = namer + "_1";
				}else{
					namer = namer + "_2";
				}

				if (paramProto[x][1] > 0) {
					testStr = (char*)namer.c_str();
					testX = pidIdx;

					strTypeParams[strParamsCount] = new StringListParameter( USTRING(namer.c_str()), pidIdx, nullptr, ParameterInfo::kCanAutomate);

					int starter;
					if (paramProto[x][1] == 4 || paramProto[x][1] == 7 ) {//osc choice and env choice
						starter = 0;
					}
					else {
						starter = 1;
					}

					for (int y = starter; y < paramProto[x][1]; y++) {
						if (y == 0) {
							strTypeParams[strParamsCount]->appendString(USTRING("0"));
						}
						else {
							strTypeParams[strParamsCount]->appendString(USTRING(std::to_string(y).c_str()));
						}
						

					}
					parameters.addParameter(strTypeParams[strParamsCount] );
					strParamsCount = strParamsCount + 1;
				}
				else {
					
					if (x == FILTER_) {
						LogScaleParameter<ParamValue>* param = new LogScaleParameter<ParamValue>(USTRING((char*)namer.c_str()), pidIdx, filterLogScale, (USTRING("Hz")));
						// NOTE: for LogScaleParameter, need to manually set default value (10kHz for filter fc)
						param->setNormalized(param->toNormalized(DEFAULT_FILTER_FC));
						param->setPrecision(2);
						parameters.addParameter(param);
					}
					else {
						addPIDParameter((char*)namer.c_str(), "%", 0, 0.0, ParameterInfo::kCanAutomate, pidIdx);
					}
					
					
					
				}
			}

			

			

		}

		for (int x = 0; x < ENV_PARAMS; x++) {
			int pidIdx = envProto[x][0];
			std::string namer = envDef[x];
			
			if (envProto[x][1] == 99) {
				//char name = (*namer.c_str());
				//addPIDParameter(envDef[x], "%", 1, 0.0, ParameterInfo::kCanAutomate, pidIdx);
				StringListParameter* slp = new StringListParameter(USTRING(namer.c_str()), pidIdx, USTRING("TRIGGER"), ParameterInfo::kCanAutomate, 0, USTRING(namer.c_str()));
				slp->appendString(USTRING("0"));
				parameters.addParameter(slp);
			}else if (envProto[x][1] > 0) {
				testStr = (char*)namer.c_str();
				testX = pidIdx;

				strTypeEnvParams[strEnvParamsCount] = new StringListParameter(USTRING(namer.c_str()), pidIdx);

				int starter = 0;
				
				for (int y = starter; y < envProto[x][1]; y++) {
					if (y == 0) {
						strTypeEnvParams[strEnvParamsCount]->appendString(USTRING("0"));
					}
					else {
						strTypeEnvParams[strEnvParamsCount]->appendString(USTRING(std::to_string(y).c_str()));
					}


				}
				parameters.addParameter(strTypeEnvParams[strEnvParamsCount]);
				strEnvParamsCount = strEnvParamsCount + 1;

			}
			else {
				addPIDParameter((char*)namer.c_str(), "%", 0, envProto[x][2], ParameterInfo::kCanAutomate, pidIdx);
			}
		}
		
		addPIDParameter("SYNC_ATTACK", "%", 0,0.0, ParameterInfo::kCanAutomate, SYNC_ATTACK);
		addPIDParameter("SYNC_DEC", "%", 0, 0.0, ParameterInfo::kCanAutomate, SYNC_DEC);
		addPIDParameter("SYNC_REL", "%", 0, 0.0, ParameterInfo::kCanAutomate, SYNC_REL);
		addPIDParameter("SYNC_TUNE", "%", 0, 0.0, ParameterInfo::kCanAutomate, SYNC_TUNE);
		addPIDParameter("SYNC_FINE", "%", 0, 0.0, ParameterInfo::kCanAutomate, SYNC_OCT);
		addPIDParameter("SYNC_INT", "%", 0, 0.0, ParameterInfo::kCanAutomate, SYNC_INT);
		addPIDParameter("SYNC_MODGAIN", "%", 0, 0.0, ParameterInfo::kCanAutomate, SYNC_MODGAIN);
		addPIDParameter("SYNC_MODOCT", "%", 0, 0.0, ParameterInfo::kCanAutomate, SYNC_MODOCT);
		addPIDParameter("SYNC_MODINT", "%", 0, 0.0, ParameterInfo::kCanAutomate, SYNC_MODINT);
		//addPIDParameter("SYNC_MODTUNE", "%", 0, 0.0, ParameterInfo::kCanAutomate, SYNC_MODTUNE);
		addPIDParameter("SYNC_MODFINE", "%", 0, 0.0, ParameterInfo::kCanAutomate, SYNC_MODFINE);
		addPIDParameter("SYNC_GAIN", "%", 0, 0.0, ParameterInfo::kCanAutomate, SYNC_GAIN);
		addPIDParameter("SYNC_MODFREAQBEAT", "%", 0, 0.0, ParameterInfo::kCanAutomate, SYNC_MODFREAQBEAT);
		
		/*
		for (int x = 0; x < PARTPARAMS; x++) {
			paramProto[x][0]
		}
		for (int x = 0; x < ENV_PARAMS; x++) {
			int pidIdx = envProto[x][0];

		}
		*/


		//others
		midiCCParamID[kCtrlModWheel] = kModWheelParam;
		addPIDParameter ("Mod Wheel", "", 0, 0, 0, kModWheelParam);
		midiCCParamID[kPitchBend] = kPitchBendParam;
		addPIDParameter ("Pitch Bend", "", 0, 0.5, 0, kPitchBendParam);


		
	}
	IMessage* ctrlrReadyMsg = allocateMessage();
	if (ctrlrReadyMsg) {
		ctrlrReadyMsg->setMessageID(&PMSG_CTRLR_READY_CHAR);
		sendMessage(ctrlrReadyMsg);
		ctrlrReadyMsg->release();
	}

	return res;
}



//-----------------------------------------------------------------------------
tresult PLUGIN_API DX10Controller::terminate ()
{
	return BaseController::terminate ();
}

void DX10Controller::controlWasChanged(int tag, double value) {
	(this->*onCtrlChange.at(tag))(tag, value);
}
//-----------------------------------------------------------------------------
tresult PLUGIN_API DX10Controller::setParamNormalized (ParamID tag, ParamValue value)//CALLED BY HOST WHEN MIDI CC
{
	//if (tag < NPARAMS) {
		//value = paramSmoothMap[tag].smoothParameter(value);
	//}
	
	tresult res;

	if (tag < GPARAMS) {
		
		int pidx = tag;
		//these are whats triggering animations,see WIDTH_REST and TUNE_RESET etc..
		// i.e. call nvgViewController->nvgView->onResetButton(pid); to run the animatio
		(this->*onCtrlChange2.at(tag))(tag, value);
		//onCtrlChange2 pure param based changes no graphics / nvgView needed
		if (nvgViewController) {
			if (nvgViewController->isCreated == true) {
				(this->*onCtrlChange.at(tag))(tag, value);
			}
		}
		


		bool skipSetParam = false;
		if (tag >= 173 ) {//not step ctrl
			skipSetParam = true;
			//res = BaseController::setParamNormalized(tag, value);
		}
		//SYNC TUNE DOESNT WORK NOW, based on set param?
		//ENVS work, BPM SYNC LFO DOESNT
		/*
		for (int i = 0; i < TOGGLE_COUNT; i++) {
			if (tag == toggleBtnPids[i]) {
				skipSetParam = true;
				break;
			}
		}*/

		if (skipSetParam == false) {
			res = BaseController::setParamNormalized(tag, value);
		}


		//res = setParamNormalized(tag, value);
	} else if (tag == kPresetParam){ 
		// preset change
		//PUT PRESET PARAM IN GPARAMS AND GET RID OF BRANCHING
		int32 program = (int32)parameters.getParameter (tag)->toPlain (value);
		for (int32 i = 0; i < GPARAMS; i++){//changed to 33
			res = BaseController::setParamNormalized (i, DX10Processor::programParams[program][i]);
			//res = setParamNormalized(i, programParams[program][i]);

		}
		componentHandler->restartComponent (kParamValuesChanged);
	}
	

	return res;
}
/*
tresult PLUGIN_API DX10Controller::notify(IMessage* message) {
	msgInt = int(*message->getMessageID());
	switch (msgInt) {
	case PMSG_SET_PARAM_NRML_CHAR:
		int64 pid;
		double val;
		message->getAttributes()->getInt("pid", pid);
		message->getAttributes()->getFloat("val", val);
		setParamNormalized(pid, val);
		return kResultTrue;
		break;
	case CMSG_GET_SAMPLE_RATE_CHAR:
		message->getAttributes()->getFloat("SampleRate", sampleRate);
		return kResultTrue;
		break;
	case CMSG_LINK_TUNE_CHAR:
		message->getAttributes()->getInt("0", linkTune);
		return kResultTrue;
		break;
	case PMSG_LFO4_RATE_CHAR:
		message->getAttributes()->getFloat("0", lfo4Rate);
		return kResultTrue;
		break;
	case PMSG_LFO5_RATE_CHAR:
		message->getAttributes()->getFloat("0", lfo5Rate);
		return kResultTrue;
		break;
	}
}
*/



int DX10Controller::safeStoi(std::wstring* Str, size_t *_Idx, int _Base){	// convert wstring to int
	int& _Errno_ref = errno;	// Nonzero cost, pay it once
	const wchar_t *_Ptr = Str->c_str();
	wchar_t *_Eptr;
	_Errno_ref = 0;
	const long _Ans = _CSTD wcstol(_Ptr, &_Eptr, _Base);
	if (_Idx != nullptr)
		*_Idx = (size_t)(_Eptr - _Ptr);
	return ((int)_Ans);
}
 
int DX10Controller::changeVertSwitch(int pid, double value) {
	//not just sending swict position for vert switches
	//but step knobs also
	//TChar* stringCtrlTchar = new TChar();
	
	EditController::getParamStringByValue(pid, value, stringCtrlTChar);//is there  beeter way to get string than like 15 lines of code?..
	stringCtrlWString = stringCtrlTChar;
	stringCtrlInt = safeStoi(&stringCtrlWString);
	
	IMessage* stringCtrlMsg = allocateMessage();
	
	if (stringCtrlMsg) {
		stringCtrlMsg->setMessageID(&PMSG_STEP_SWITCH_CHAR);
		stringCtrlMsg->getAttributes()->setInt("0", stringCtrlInt);
		stringCtrlMsg->getAttributes()->setInt("1", pid);
		sendMessage(stringCtrlMsg);
		stringCtrlMsg->release();
	}
	
	return 0;
}

int DX10Controller::changeLFORate(int pid, double value) {
	//not just sending swict position for vert switches
	//but step knobs also
	//TChar* stringCtrlTchar = new TChar();

	IMessage* stringCtrlMsg = allocateMessage();

	if (stringCtrlMsg) {
		stringCtrlMsg->setMessageID(&PMSG_LFO_RATE_CHAR);
		stringCtrlMsg->getAttributes()->setFloat("0", value);
		stringCtrlMsg->getAttributes()->setInt("1", pid);
		sendMessage(stringCtrlMsg);
		stringCtrlMsg->release();
	}

	return 0;
}



//-----------------------------------------------------------------------------
tresult PLUGIN_API DX10Controller::getParamStringByValue (ParamID tag, ParamValue valueNormalized, String128 string)
{
	return BaseController::getParamStringByValue (tag, valueNormalized, string);
	
	/*
	UString128 result;
		switch (tag)
		{
			// TODO
			default:
				return BaseController::getParamStringByValue (tag, valueNormalized, string);
		}
		result.copyTo (string, 128);
		return kResultTrue;*/
	
}






}}} // namespaces
