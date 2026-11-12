
#pragma once


#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/base/ustring.h"
#include "mdaParameter.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "RSView.h"
#include <cuchar>
#include "logscale.h"
#include "SynthParamLimits.h"
#include "CParamSmooth.h"
#include "constants.h"

namespace Steinberg {
namespace Vst {
namespace rs {

#define FILTER1_FC 100
#define FILTER2_FC 200
#define FILTER1_Q 101
#define FILTER2_Q 201

//-----------------------------------------------------------------------------
class BaseController : public EditControllerEx1, public IMidiMapping, public VSTGUI::VST3EditorDelegate
{
public:
	BaseController();
	int msgInt;
	//int linkTune = 0;
	int64 linkTune = 0;

	std::wstring valueFromRegistry;

	const char* regErr1;

	bool regKeyFound;

	bool regKeyLegit;

	double lfo4Rate = 0.0;
	double lfo5Rate = 0.0;
	double lfo6Rate = 0.0;

	bool badReg = false;

	Map::hash<char*, int> pidMap;
	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API setComponentState (IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API notify (IMessage* message) SMTG_OVERRIDE;
	//virtual IPlugView* PLUGIN_API createView(FIDString name);

	//void valueChanged(CControl* control) override;

	CRect viewRect;
	void addPIDParameter(char * titler, char * units, int32 stepCount, ParamValue defaultNormalizedValue, int32 flags, int32 tag);
	void addPIDParameter(char* titler, char* units, int32 stepCount, ParamValue defaultNormalizedValue,
		int32 flags, int32 tag, int min, int max, bool isTrue);

	std::u16string StringtoU16(const std::string & str);

	tresult PLUGIN_API sendParamKeys();
	int32 PLUGIN_API getProgramListCount () SMTG_OVERRIDE;
	tresult PLUGIN_API getProgramListInfo (int32 listIndex, ProgramListInfo& info /*out*/) SMTG_OVERRIDE;
	tresult PLUGIN_API getProgramName (ProgramListID listId, int32 programIndex, String128 name /*out*/) SMTG_OVERRIDE;
	tresult PLUGIN_API getMidiControllerAssignment (int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& tag/*out*/) SMTG_OVERRIDE;
	
	ParameterContainer& getParameters () { return parameters; }

	tresult beginEdit(Steinberg::Vst::ParamID tag) override;
	tresult performEdit(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized) override;
	tresult endEdit(Steinberg::Vst::ParamID tag) override;
	Parameter* getParameterObject(Steinberg::Vst::ParamID tag) override;

	//IController* createSubController(const char* name, const IUIDescription* description, VST3Editor* editor) override;

	bool isPrivateParameter(const Steinberg::Vst::ParamID paramID) override;
	bool sv = false;

	RSView* rsView = NULL;
	//CRect viewRect;
	//RSView rsViewObj;
	//NVGViewController nvgViewControllerObj;
	//NVGViewController* nvgViewController = NULL;
	//std::unique_ptr<NVGViewController> nvgViewController;
protected:
	Steinberg::Vst::ParameterContainer uiParameters;
	double output;
	double envVal;
	double sinLev;
	double viewWidth;
	double viewHeight;
	
	

	//-----------------------------
	DELEGATE_REFCOUNT (EditControllerEx1)
	tresult PLUGIN_API queryInterface (const char* iid, void** obj) SMTG_OVERRIDE;
	//-----------------------------

	enum {
		kMagicNumber = 9999999,

		kBypassParam = 'bpas',
		kPresetParam = 'prst',
		kModWheelParam = 'modw',
		kBreathParam = 'brth',
		kCtrler3Param = 'ct03',
		kExpressionParam = 'expr',
		kPitchBendParam = 'pitb',
		kSustainParam = 'sust',
		kAftertouchParam = 'aftt',
		kLinkGen1Param = 'lg1p',
	};

	static const TChar kMicroSecondsString[];

	virtual void doFromBase(int pid, double value) {};

	virtual void setViewsFromBase() {};

	int (BaseController::*onNotif[20])(IMessage* message);
	//int cmsgOnStepSwitch(IMessage* message);
	int cmsgOnLinkCtrl(IMessage* message);
	int cmsgOnGetSampleRate(IMessage* message);






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
	//PARAMDEF MUST BE THE SAME AS IN NVGVIEW
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
	//"MOD_GAIN_RESET",
	"MOD_WIDTH_RESET",
	"MOD_TUNE_RESET",
	"MOD_FINE_RESET",
	//"TOGGLE_GEN_VIEW",
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
		{(float)ENV_1_ATTACK, 0, 0.05},
		{(float)ENV_1_DECAY, 0, 0.82},
		{(float)ENV_1_RELEASE, 0, 0.82},
		{(float)ENV_1_DECAYTOGGLE, 0,0.0},
		{(float)ENV_1_DELAY, 0, 0.0},
		{(float)ENV_2_ATTACK, 0, 0.05},
		{(float)ENV_2_DECAY, 0, 0.82},
		{(float)ENV_2_RELEASE, 0, 0.82},
		{(float)ENV_2_DECAYTOGGLE, 0,0.0},
		{(float)ENV_2_DELAY, 0, 0.0},
		{(float)ENV_3_ATTACK, 0, 0.05},
		{(float)ENV_3_DECAY, 0, 0.82},
		{(float)ENV_3_RELEASE, 0, 0.82},
		{(float)ENV_3_DECAYTOGGLE, 0,0.0},
		{(float)ENV_3_DELAY, 0, 0.0},
		{(float)LFO_1_RATE, 0, 0.25},
		{(float)LFO_1_AMP, 0, 0.25},
		{(float)LFO_1_TYPE, 4, 0.0},
		{(float)LFO_1_BPM, 0, 0.0},
		{(float)LFO_1_DELAY, 0, 0.0},
		{(float)LFO_2_RATE, 0, 0.25},
		{(float)LFO_2_AMP, 0, 0.85},
		{(float)LFO_2_TYPE, 4, 0.85},
		{(float)LFO_2_BPM, 0, 0.0},
		{(float)LFO_2_DELAY, 0, 0.0},
		{(float)LFO_3_RATE, 0, 0.25},
		{(float)LFO_3_AMP, 0, 0.25},
		{(float)LFO_3_TYPE, 4, 0.0},
		{(float)LFO_3_BPM, 0, 0.0},
		{(float)LFO_3_DELAY, 0, 0.0},
		{(float)WIDTH_RESET, 0, 0.0},
		{(float)TUNE_RESET, 0, 0.0},
		{(float)FINE_RESET, 0, 0.0},
		//{(float)MOD_GAIN_RESET, 0, 0.0},
		{(float)MOD_WIDTH_RESET, 0, 0.0},
		{(float)MOD_TUNE_RESET, 0, 0.0},//
		{(float)MOD_FINE_RESET, 0, 0.0},
		//{(float)TOGGLE_GEN_VIEW, 0, 0.0},
		{(float)LFO_1_PHASE, 0, 0.0},
		{(float)LFO_2_PHASE, 0, 0.0},
		{(float)LFO_3_PHASE, 0, 0.0},
		{(float)STEP_CTRL_TARG, 8, 0.0},
		{(float)STEP_CTRL_1, 99, 0.0},
		{(float)STEP_CTRL_2, 99, 0.0},
		{(float)STEP_CTRL_3, 99, 0.0},
		{(float)STEP_CTRL_4, 99, 0.0},
		{(float)STEP_CTRL_5, 99, 0.0},
		{(float)STEP_CTRL_6, 99, 0.0},
		{(float)STEP_CTRL_7, 99, 0.0},
		{(float)STEP_CTRL_8, 99, 0.0},
		{(float)STEP_CTRL_9, 99, 0.0},
		{(float)STEP_CTRL_10, 99, 0.0},
		{(float)STEP_CTRL_11, 99, 0.0},
		{(float)STEP_CTRL_12, 99, 0.0}
		 
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
		//MOD_GAIN_RESET,
		MOD_WIDTH_RESET,
		MOD_TUNE_RESET,
		MOD_FINE_RESET,
		//TOGGLE_GEN_VIEW,
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

	int safeStoi(std::wstring* Str, size_t *_Idx = nullptr, int _Base = 10);

	String128 stringCtrlTChar;

	int stringCtrlInt;

	std::wstring stringCtrlWString;


protected:
	double getSampleRate () const { return sampleRate; }
	int32 midiCCParamID[kCountCtrlNumber];
	double sampleRate;
	bool addBypassParameter;
};

}}} 
