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
#include "editorLayout.h"
#include "paramDisplay.h"
#include "lfoShape.h"
#include "fxStrip.h"
#include <cmath>
#include <cstdio>




namespace Steinberg {
namespace Vst {
namespace rs {

	LogScale<ParamValue> filterLogScale(MIN_UNIPOLAR,	/* VST GUI Variable MIN (0) */
		MAX_UNIPOLAR,	/* VST GUI Variable MAX (1) */
		MIN_FILTER_FC,	/* filter fc LOW */
		MAX_FILTER_FC,	/* filter fc HIGH */
		FILTER_RAW_MAP,/* the value at position 0.5 will be: */
		FILTER_COOKED_MAP);/* 1800 Hz */

	// POLY_COUNT: 1..POLY_MAX voices, 64 steps; the host shows and enters the number.
	class PolyParameter : public Parameter
	{
	public:
		explicit PolyParameter (ParamID tag)
		: Parameter (USTRING ("POLYPHONY"), tag, USTRING ("voices"), polyToNormalized (POLY_DEFAULT), POLY_MAX - 1,
		             ParameterInfo::kCanAutomate)
		{
		}

		void toString (ParamValue valueNormalized, String128 string) const SMTG_OVERRIDE
		{
			char buf[16];
			snprintf (buf, sizeof (buf), "%d", polyFromNormalized (valueNormalized));
			UString (string, 128).fromAscii (buf);
		}

		bool fromString (const TChar* string, ParamValue& valueNormalized) const SMTG_OVERRIDE
		{
			UString wrapper ((TChar*)string, strlen16 (string));
			int64 n = 0;
			if (!wrapper.scanInt (n))
				return false;
			valueNormalized = polyToNormalized (static_cast<int> (n));
			return true;
		}

		ParamValue toPlain (ParamValue valueNormalized) const SMTG_OVERRIDE { return polyFromNormalized (valueNormalized); }
		ParamValue toNormalized (ParamValue plainValue) const SMTG_OVERRIDE
		{
			return polyToNormalized (static_cast<int> (plainValue + 0.5));
		}
	};

	// SNAP_TIME: the retrigger/steal fade in ms (2..80, log; constants.h snapMs).
	class SnapParameter : public Parameter
	{
	public:
		explicit SnapParameter (ParamID tag)
		: Parameter (USTRING ("SNAP"), tag, USTRING ("ms"), snapToNormalized (SNAP_DEFAULT_MS), 0,
		             ParameterInfo::kCanAutomate)
		{
		}

		void toString (ParamValue valueNormalized, String128 string) const SMTG_OVERRIDE
		{
			char buf[16];
			snprintf (buf, sizeof (buf), "%.1f", snapMs (valueNormalized));
			UString (string, 128).fromAscii (buf);
		}

		bool fromString (const TChar* string, ParamValue& valueNormalized) const SMTG_OVERRIDE
		{
			UString wrapper ((TChar*)string, strlen16 (string));
			double ms = 0.0;
			if (!wrapper.scanFloat (ms) || !(ms > 0.0))
				return false;
			valueNormalized = (std::min) (1.0, (std::max) (0.0, snapToNormalized (ms)));
			return true;
		}

		ParamValue toPlain (ParamValue valueNormalized) const SMTG_OVERRIDE { return snapMs (valueNormalized); }
		ParamValue toNormalized (ParamValue plainValue) const SMTG_OVERRIDE
		{
			return plainValue > 0.0 ? (std::min) (1.0, (std::max) (0.0, snapToNormalized (plainValue))) : 0.0;
		}
	};

	// MASTER_VOL: the host shows/enters dB (paramDisplay::masterVolText; gain = 2 v^2).
	class MasterVolParameter : public Parameter
	{
	public:
		explicit MasterVolParameter (ParamID tag)
		: Parameter (USTRING ("MASTER VOLUME"), tag, USTRING ("dB"), DEFAULT_MASTER_VOL, 0,
		             ParameterInfo::kCanAutomate)
		{
		}

		void toString (ParamValue valueNormalized, String128 string) const SMTG_OVERRIDE
		{
			std::string s = paramDisplay::masterVolText (valueNormalized);
			const std::string::size_type unit = s.find (" dB");  // the unit is shown separately
			if (unit != std::string::npos)
				s.erase (unit);
			UString (string, 128).fromAscii (s.c_str ());
		}

		bool fromString (const TChar* string, ParamValue& valueNormalized) const SMTG_OVERRIDE
		{
			UString wrapper ((TChar*)string, strlen16 (string));
			double db = 0.0;
			if (!wrapper.scanFloat (db))
			{
				valueNormalized = 0.0;  // "-inf" or anything else that is not a number: silence
				return true;
			}
			if (db > 6.0) db = 6.0;
			valueNormalized = (db < -100.0) ? 0.0 : std::sqrt (std::pow (10.0, db / 20.0) * 0.5);
			return true;
		}

		OBJ_METHODS (MasterVolParameter, Parameter)
	};

// (the factory preset names: factoryPresetName, mdaDX10Presets.cpp)

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

//somethng related to is view ready is blocking some of the controls vrom being set in the controller 
bool DX10Controller::isViewReady()  {
	if (nvgViewReady) {
		return true;

	}
	return false;
	
}

IController* DX10Controller::createSubController(const char* name, const IUIDescription* description, VST3Editor* editor) {
	if (strcmp(name, "nvgViewController") == 0  ) {
		ParameterContainer& pcr = getParameters();

		nvgViewController = new NVGViewController(editor, this);
		if (!nvgViewController || !nvgViewController->nvgView) {////
			delete nvgViewController;
			nvgViewController = nullptr;
			return nullptr; // Return early if initialization fails
		}

		//nvgViewController->nvgView->baseController = this;
		nvgViewController->nvgView->lfo1Rate = &lfo4Rate;
		nvgViewController->nvgView->lfo2Rate = &lfo5Rate;
		nvgViewController->nvgView->lfo3Rate = &lfo6Rate;

		
		//NVGParamDef is now just duplicated in DX10Processor and NVGView
		//for (int i = 0; i < PARTPARAMS; i++) {
		//	nvgViewController->nvgView->NVGParamDef[i] = ParamDef[i];
		//}

		nvgViewController->nvgView->partParamsReady = true;

		// licensing v1: look again (a license may have been pasted in another instance); NVGView shows
		// the registration screen from baseController->licensed. If it is licensed now, the processor
		// is told to look too.
		refreshLicense();
		if (licensed)
			notifyLicenseChanged();

		modTypeNrml = EditController::getParamNormalized(MOD_TYPE_);
		EditController::getParamStringByValue(MOD_TYPE_, modTypeNrml, modTypeCtrlTChar);//is there  beeter way to get string than like 15 lines of code?..
		modTypeCtrlWString = modTypeCtrlTChar;
		modTypeCtrlInt = safeStoi(&modTypeCtrlWString);
		if (nvgViewController) {
			nvgViewController->nvgView->onChangeModType(MOD_TYPE_, modTypeCtrlInt);
		}

	
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


void DX10Controller::checkPendingPreset() {
	if (presetPending && isViewReady()) {
		DX10Processor::ensureFactoryPresets();  // B-11
		for (int32 i = 0; i < GPARAMS; i++) {
			if (isGlobalParam(i)) continue;  // global, not part of presets
			BaseController::setParamNormalized(i, DX10Processor::programParams[pendingPresetIndex][i]);
		}
		componentHandler->restartComponent(kParamValuesChanged);
		presetPending = false; // Clear the flag
	}
}
//MAKE OUR EDITOR
IPlugView* PLUGIN_API DX10Controller::createView(FIDString _name) {
	ConstString name(_name);
	if (name == ViewType::kEditor) {
		// --- create the editor using the .uidesc file (in our Resources, edited with WYSIWYG editor in client)
		
		CRect viewRect(0, 0, ::editorLayout::kEditorWidth, ::editorLayout::kEditorHeight);
		//rsView = new RSView(this, "Editor", "vstgui.uidesc", viewRect);
		//return rsView;

		return new RSView(this, "Editor", "vstgui.uidesc", viewRect);
		
		//return new VSTGUI::VST3Editor(this, "Editor", "vstgui.uidesc");
	}
	checkPendingPreset();
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
	onCtrlChange[MASTER_VOL] = &DX10Controller::onEnvBipolar;  // readout (envName -> "VOLUME")
	onCtrlChange[POLY_COUNT] = &DX10Controller::onEnvBipolar;  // readout "POLYPHONY / n VOICES"
	onCtrlChange[SNAP_TIME] = &DX10Controller::onEnvBipolar;   // readout "SNAP / 21.0 MS"
	onCtrlChange[EXPR_AMOUNT] = &DX10Controller::onEnvBipolar; // readout "EXPRESSION / +40 % BRIGHT"
	onCtrlChange[EXPR_TARGET] = &DX10Controller::onEnvBipolar; // readout "EXPR TARGET / FILTER"
	onCtrlChange[EXPR_VEL] = &DX10Controller::onToggleButton;  // button glow, readout ON/OFF
	onCtrlChange[DRIFT_AMOUNT] = &DX10Controller::onEnvBipolar; // readout "DRIFT / 40 % (6 CENTS)"
	onCtrlChange[HQ_MODE] = &DX10Controller::onToggleButton;    // button glow, readout ON/OFF
	onCtrlChange[KEY_SYNC] = &DX10Controller::onToggleButton;   // button glow, readout ON/OFF
	for (int id = FX_PARAM_FIRST; id <= FX_PARAM_LAST; id++)
		onCtrlChange[id] = &DX10Controller::onEnvBipolar;       // readout (paramDisplay)

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
	// B-59: reset only when the button is pressed (1). This handler runs for every value the reset
	// params get: the release (0), every preset/state load and the editor-open replay. Each of those
	// re-centred WIDTH/TUNE/FINE and the MOD ones with a performEdit, overwriting the loaded values.
	if (msg < 0.5)
		return 0;
	if (nvgViewController && nvgViewController->nvgView)
		nvgViewController->nvgView->onResetButton(pid);
	return 0;
}


// The readout handlers below show the value with units (paramDisplay); they used to show the raw
// normalized (or bipolar) number.
int DX10Controller::onEnvBipolar(int pid, double msg) {
	if (nvgViewController && nvgViewController->nvgView)
		nvgViewController->nvgView->onEnvBipolar(pid, readoutValue(pid, msg));
	return 0;
}

int DX10Controller::onEnv(int pid, double msg) {
	if (nvgViewController && nvgViewController->nvgView)
		nvgViewController->nvgView->onEnv(pid, readoutValue(pid, msg));
	return 0;
}

int DX10Controller::onPanEnv(int pid, double msg) {
	if (nvgViewController && nvgViewController->nvgView)
		nvgViewController->nvgView->onPanEnv(pid, readoutValue(pid, msg));
	return 0;
}

int DX10Controller::onEnvLev(int pid, double msg) {
	if (nvgViewController && nvgViewController->nvgView)
		nvgViewController->nvgView->onEnvLev(pid, readoutValue(pid, msg));
	return 0;
}

int DX10Controller::stepValue(int pid, double normalized) {
	// Same conversion as changeVertSwitch: the engine gets the parameter's string as an integer.
	String128 str;
	if (EditController::getParamStringByValue(pid, normalized, str) != kResultTrue)
		return 0;
	std::wstring w = str;
	return safeStoi(&w);
}

std::string DX10Controller::readoutValue(int pid, double normalized) {
	paramDisplay::Context ctx;
	if (Parameter* p = getParameterObject(pid)) {
		if (p->getInfo().stepCount > 0)
			ctx.step = stepValue(pid, normalized);
	}
	const int srcPid = paramDisplay::sourceParamFor(pid);
	if (srcPid >= 0)
		ctx.srcStep = stepValue(srcPid, getParamNormalized(srcPid));
	const int syncPid = paramDisplay::syncParamFor(pid);
	if (syncPid >= 0)
		ctx.lfoSynced = getParamNormalized(syncPid) >= 1.0;  // processor: syncBpm = (int)value
	if (pid == EXPR_AMOUNT) {
		ctx.exprTarget = exprTargetFromNormalized(getParamNormalized(EXPR_TARGET));
		ctx.exprVel = getParamNormalized(EXPR_VEL) >= 0.5;
	}
	if (pid >= LFO_1_PHASE && pid <= LFO_3_PHASE)
		ctx.lfoType = lfoShape::typeFromNormalized(getParamNormalized(LFO_1_TYPE + 5 * (pid - LFO_1_PHASE)));
	std::string s = paramDisplay::value(pid, normalized, ctx);
	if (s.empty()) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%.3f", normalized);
		s = buf;
	}
	return s;
}

bool DX10Controller::showParamReadout(int pid) {
	if (pid < 0 || pid >= GPARAMS || !isViewReady() || !nvgViewController || !nvgViewController->nvgView)
		return false;
	const auto handler = onCtrlChange[pid];
	const double value = getParamNormalized(pid);
	// Only the handlers that just update the readout; reset/step buttons, the step target and
	// toggle animations have side effects and are not run on hover.
	if (handler == &DX10Controller::onEnv || handler == &DX10Controller::onEnvBipolar ||
	    handler == &DX10Controller::onPanEnv || handler == &DX10Controller::onEnvLev ||
	    handler == &DX10Controller::onTuneSwitch || handler == &DX10Controller::onEnvSrc ||
	    handler == &DX10Controller::onChangeOSCType || handler == &DX10Controller::onChangeLFOOSCType ||
	    handler == &DX10Controller::onChangeModType) {
		(this->*handler)(pid, value);
		return true;
	}
	if (handler == &DX10Controller::onToggleButton) {
		nvgViewController->nvgView->onOnOffBtn(pid, static_cast<int>(value));
		return true;
	}
	// The reset squares: say what a click does (hovering does not run widthReset).
	if (pid >= WIDTH_RESET && pid <= MOD_FINE_RESET && paramDisplay::label(pid)[0]) {
		nvgViewController->nvgView->onEnv(pid, "CLICK: CENTRE");
		return true;
	}
	return false;
}

int DX10Controller::onEnvSrc(int pid, double msg) {
	selectedEnvSrc = static_cast<int>((msg * 100.0) * 1 / 14.35);

		nvgViewController->nvgView->onEnvSrc(pid, selectedEnvSrc);
		//changeVertSwitch(pid, msg);
		//call changeVertSwitch bc we're overwring ctrl event handler target for changing processer params where this was called
	
	return 0;
}

int DX10Controller::onTuneSwitch(int pid, double msg) {
	// Was msg * 100 / divisor, which did not match the step the engine gets (e.g. OCT_ showed 0..4
	// for six positions); readoutValue uses the parameter's own string.
	if (nvgViewController && nvgViewController->nvgView)
		nvgViewController->nvgView->onTuneSwitch(pid, readoutValue(pid, msg));
	return 0;
}

int DX10Controller::onChangeOSCType(int pid, double msg) {
	selectedModType = static_cast<int>((msg * 100.0) * 1 / 25);
	
	nvgViewController->nvgView->onChangeOSCType(pid, selectedModType);
		//changeVertSwitch(pid, msg);
	
	return 0;
}

int DX10Controller::onChangeLFOOSCType(int pid, double msg) {
	selectedModType = static_cast<int>((msg * 100.0) * 1 / 25);

		nvgViewController->nvgView->onChangeLFOOSCType(pid, selectedModType);
		//changeVertSwitch(pid, msg);
	
	return 0;
}

int DX10Controller::onChangeModType(int pid, double msg) {
	selectedModType = static_cast<int>(((msg * 100.0) * 1 / 25)-0.01);
	
		nvgViewController->nvgView->onChangeModType(pid, selectedModType);
		//changeVertSwitch(pid, msg);
	
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
	if (isViewReady() == false ) {
		return 0;
	}
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
		if (nvgViewController) {
			if (nvgViewController->isCreated == true) {
				nvgViewController->nvgView->onStepCtrlButton(pid);
			}
		}
	}
	
	return 0;
}

int DX10Controller::onToggleButton(int pid, double msg) {
	int stater = static_cast<int>(msg);
			
		nvgViewController->nvgView->onOnOffBtn(pid, stater);
		nvgViewController->nvgView->onToggleButton(pid);
	
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
	if (nvgViewController) {
		//delete nvgViewController;
		//nvgViewController = nullptr;
	}
}


void DX10Controller::doFromBase(int pid, double value) {
	if (isViewReady()) {
		(this->*onCtrlChange.at(pid))(pid, value);
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
			presetParam->setIndexString(i, UString128(factoryPresetName(i)));
			
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
						// B-57: also the host's default (reset); the constructor leaves it at 0 (80 Hz)
						param->getInfo().defaultNormalizedValue = param->toNormalized(DEFAULT_FILTER_FC);
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
				//step control
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

		parameters.addParameter (new MasterVolParameter (MASTER_VOL));
		parameters.addParameter (new PolyParameter (POLY_COUNT));
		parameters.addParameter (new SnapParameter (SNAP_TIME));
		parameters.addParameter (USTRING ("EXPRESSION"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, EXPR_AMOUNT);
		{
			auto* t = new StringListParameter (USTRING ("EXPR TARGET"), EXPR_TARGET);
			for (int i = 0; i < kExprTargetCount; i++)
				t->appendString (USTRING (exprTargetName (i)));
			parameters.addParameter (t);
		}
		parameters.addParameter (USTRING ("EXPR VELOCITY"), nullptr, 1, 0.0, ParameterInfo::kCanAutomate, EXPR_VEL);
		parameters.addParameter (USTRING ("DRIFT"), USTRING ("%"), 0, 0.0, ParameterInfo::kCanAutomate, DRIFT_AMOUNT);
		parameters.addParameter (USTRING ("HQ (2x OVERSAMPLING)"), nullptr, 1, 0.0, ParameterInfo::kCanAutomate, HQ_MODE);
		parameters.addParameter (USTRING ("OSC KEY SYNC"), nullptr, 1, 0.0, ParameterInfo::kCanAutomate, KEY_SYNC);
		{
			auto* t = new StringListParameter (USTRING ("DELAY TIME"), DLY_TIME);
			for (int i = 0; i < fx::kDelayDivisions; i++)
				t->appendString (USTRING (fx::delayDivisionName (i)));
			t->setNormalized (7.0 / 11.0);
			t->getInfo ().defaultNormalizedValue = 7.0 / 11.0;
			parameters.addParameter (t);
		}
		parameters.addParameter (USTRING ("DELAY FEEDBACK"), USTRING ("%"), 0, 0.35, ParameterInfo::kCanAutomate, DLY_FDBK);
		parameters.addParameter (USTRING ("DELAY TONE"), USTRING ("%"), 0, 0.6, ParameterInfo::kCanAutomate, DLY_TONE);
		parameters.addParameter (USTRING ("DELAY MIX"), USTRING ("%"), 0, 0.0, ParameterInfo::kCanAutomate, DLY_MIX);
		parameters.addParameter (USTRING ("CHORUS RATE"), USTRING ("%"), 0, 0.4, ParameterInfo::kCanAutomate, CHO_RATE);
		parameters.addParameter (USTRING ("CHORUS DEPTH"), USTRING ("%"), 0, 0.5, ParameterInfo::kCanAutomate, CHO_DEPTH);
		parameters.addParameter (USTRING ("CHORUS MIX"), USTRING ("%"), 0, 0.0, ParameterInfo::kCanAutomate, CHO_MIX);
		
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
		ctrlrReadyMsg->setMessageID(msgIdStr(PMSG_CTRLR_READY_CHAR));  // B-27
		ctrlrReadyMsg->getAttributes()->setInt("id", this->myInstanceId);
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
	if (isViewReady()) {
		(this->*onCtrlChange.at(tag))(tag, value);
	}
}
//-----------------------------------------------------------------------------
tresult PLUGIN_API DX10Controller::setParamNormalized (ParamID tag, ParamValue value)//CALLED BY HOST WHEN MIDI CC
{
	tresult res = 0;

	if (tag < GPARAMS) {
		// B-02: values from the host (automation, MIDI CC, presets, state) are always stored and the
		// engine is always kept in sync; only the GUI part waits for the editor.
		// onCtrlChange2 = engine sync (vertical switches: sends the integer position to the
		// processor, which only gets ctrlValues this way); onCtrlChange = GUI handlers (nvgView).
		(this->*onCtrlChange2.at(tag))(tag, value);

		if (isViewReady()) {//not enough to know if the things the the view are really ready
			(this->*onCtrlChange.at(tag))(tag, value);

			if (isHostParam(int(tag))) {

				res = BaseController::setParamNormalized(tag, value);
			}
			else {
				onStepCtrlButton(tag, value);
			}

		}
		else if (isHostParam(int(tag))) {
			res = BaseController::setParamNormalized(tag, value);
			// the GUI catches up from these in onNVGViewReady() when the editor opens.
			// (tags >= STEP_BUTTONS_FIRST are momentary step buttons; their handler performs an edit, so they are
			// not replayed.)
			pendingStateUpdates[tag] = value;
			hasPendingState = true;
		}

	} else if (tag == kPresetParam){
		// B-01: apply the preset now, editor open or not (it used to be parked in presetPending,
		// which nothing ever applied, so the controller kept the old values).
		res = BaseController::setParamNormalized(tag, value);
		int32 program = (int32)parameters.getParameter(tag)->toPlain(value);
		program = std::max<int32>(0, std::min<int32>(DX10Processor::kNumPrograms - 1, program));
		const bool viewReady = isViewReady();
		DX10Processor::ensureFactoryPresets();  // B-11: the table may not exist yet without a processor
		for (int32 i = 0; i < GPARAMS; i++) {
			if (isGlobalParam(i)) continue;  // global, not part of presets (the processor keeps them too)
			const ParamValue v = DX10Processor::programParams[program][i];
			BaseController::setParamNormalized(i, v);
			if (!viewReady && isHostParam(i) && EditController::getParameterObject(i)) {
				pendingStateUpdates[i] = v;  // GUI catches up when the editor opens
				hasPendingState = true;
			}
		}
		presetPending = false;
		// preset bar: the host picked a factory program
		presetFactory = program;
		presetPath.clear();
		presetName = factoryPresetName(program);
		presetDirty = false;
		syncPresetCursor();
		if (componentHandler)
			componentHandler->restartComponent(kParamValuesChanged);
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
		stringCtrlMsg->setMessageID(msgIdStr(PMSG_STEP_SWITCH_CHAR));  // B-27
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
		stringCtrlMsg->setMessageID(msgIdStr(PMSG_LFO_RATE_CHAR));  // B-27
		stringCtrlMsg->getAttributes()->setFloat("0", value);
		stringCtrlMsg->getAttributes()->setInt("1", pid);
		stringCtrlMsg->getAttributes()->setInt("id", this->myInstanceId);
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
