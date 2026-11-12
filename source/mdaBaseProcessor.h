
#ifndef __BASEPROCESSOR_H_INCLUDED__
#define __BASEPROCESSOR_H_INCLUDED__


#include "DXGenerator.h"
//#include "Map.h"

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "constants.h"
#include <math.h>
#include <bcrypt.h>

namespace Steinberg {
namespace Vst {
namespace rs {

//-----------------------------------------------------------------------------
class BaseProcessor : public AudioEffect
{
public:
	BaseProcessor ();
	~BaseProcessor ();

	enum {
		
		kNumPrograms = 7,
		//GENCOUNT = 2,
		EVENTBUFFER = 1024,//expand?//or shrink?was 240
		EVENTS_DONE = 99999999,
		NVOICES = (PARTVOICES * 2),       //max polyphony
		SUSTAIN = 128
	};

	int64 myInstanceId = 0;

	ParamValue* params;
	ParamValue floatParams[GPARAMS];

	//ParamValue* floatParamsP[32];

	//std::vector<ParamValue> genParams;
	ParamValue* gen1Params;
	ParamValue* gen2Params;

	

	DXGenerator generators[GENCOUNT];
	void recalculate();

	int32 genRecalcIdx = 0;
	int32 genCountIdx = 0;

	
	virtual void doProcessing (ProcessData& data) = 0;
	virtual bool bypassProcessing (ProcessData& data);
	virtual void processEvents(IEventList* /*events*/) {};
	virtual void preProcess(ProcessData& data) {};
	virtual void checkSilence (ProcessData& data);
	virtual void setBypass (bool state, int32 sampleOffset);
	virtual bool processParameterChanges (IParameterChanges* changes);
	virtual void setParameter (ParamID index, ParamValue newValue, int32 sampleOffset);
	virtual void allocParameters (int32 numParams);
	tresult PLUGIN_API sendDoubleAttr(const char* msgID, const char* attrID, const double& value);
	tresult PLUGIN_API sendPidVal(const char* msgID, const double& pid, const double& val );
	tresult PLUGIN_API sendIntAttr(const char* msgID, const char* attrID, const int& value);
	//tresult PLUGIN_API sendDoubleAttr(const int* msgID, const char*, const double& value);

	virtual bool hasProgram () const { return false; }
	virtual uint32 getCurrentProgram () const { return 0; }
	virtual void setCurrentProgram (uint32 val) {}
	virtual void setCurrentProgramNormalized (ParamValue val) {}

	bool isBypassed () const { return bypassState; }
	double getSampleRate () const { return processSetup.sampleRate; }

	tresult PLUGIN_API notify(IMessage* message) SMTG_OVERRIDE;

	tresult PLUGIN_API process (ProcessData& data) SMTG_OVERRIDE;

	tresult PLUGIN_API setupProcessing (ProcessSetup& setup) SMTG_OVERRIDE;
	tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE;
	tresult PLUGIN_API setBusArrangements (SpeakerArrangement* inputs, int32 numIns,
	                                       SpeakerArrangement* outputs, int32 numOuts) SMTG_OVERRIDE;

	tresult PLUGIN_API setState (IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE;


	uint32 numParams;
	int32 bypassRamp;

	float* bypassBuffer0;
	float* bypassBuffer1;

	bool bypassState;


	Map::hash<std::string, int> paramKeys;
	void addParameter(char * titler, char * units, int32 stepCount, ParamValue defaultNormalizedValue, int32 flags, int32 tag);
	
	int vertSwitchPids[_GEN_STEP_PARAM_COUNT_] = { OSC_, WIDTH_ENV_SRC, TUNE_ENV_SRC, FINE_ENV_SRC, OCT_, OCT_ENV_SRC, INT_, INT_ENV_SRC, MOD_OSC_, MOD_TYPE_, MOD_GAIN_ENV_SRC, MOD_WIDTH_ENV_SRC, MOD_TUNE_ENV_SRC, MOD_FINE_ENV_SRC, MOD_OCT_, MOD_OCT_ENV_SRC, MOD_INT_, MOD_INT_ENV_SRC, MOD_BEAT_, MOD_BEAT_ENV_SRC, GAIN_ENV_SRC, FILTER_ENV_SRC, PAN_LR_ENV_SRC };


	///////////////////////////////

	//THIS IS STARTUP SOUND
	/*
	const float presetParams[NPARAMS] = {
		0.0, //Osc Type
		0.5, //width
		0.5, //w_env
		0.0, //w_env_s
		0.5, //Tune
		0.5, //Tune env
		0.0, //TuneEnvSource
		0.5, //fine
		0.5, //
		0.0, //
		0.4, //oct
		0.5, //
		0.0, //
		0.0, //int
		0.5, //
		0.0, //
		0.0, //Mod osc
		0.0, //Mod ty
		0.0, //gain
		0.5, //
		0.0, //
		0.5, //width
		0.5, //
		0.0, //
		0.5, //tune
		0.5, //
		0.0, //
		0.5,//fine
		0.5, //
		0.0, //
		0.4, //oct
		0.5, //
		0.0, //
		0.0, //int
		0.5, //
		0.0, //
		0.0, //beat
		0.5, //
		0.0, //
		0.75, //gain
		0.5,//
		0.0,//
		0.0,//filter
		0.5,//
		0.0,//
		0.5,//triW
		0.5,//modtriW
		0.0,
		0.0,
		0.0,//99

		0.0, //Osc Type
		0.5, //width
		0.5, //w_env
		0.0, //w_env_s
		0.5, //Tune
		0.5, //Tune env
		0.0, //TuneEnvSource
		0.5, //fine
		0.5, //
		0.0, //
		0.4, //oct
		0.5, //
		0.0, //
		0.0, //int
		0.5, //
		0.0, //
		0.0, //Mod osc
		0.0, //Mod ty
		0.0, //gain
		0.5, //
		0.0, //
		0.5, //width
		0.5, //
		0.0, //
		0.5, //tune
		0.5, //
		0.0, //
		0.5,//fine
		0.5, //
		0.0, //
		0.4, //oct
		0.5, //
		0.0, //
		0.0, //int
		0.5, //
		0.0, //
		0.0, //beat
		0.5, //
		0.0, //
		0.75, //gain
		0.5,//
		0.0,//
		0.0,//filter
		0.5,//
		0.0,//
		0.5,//triW
		0.5,//modtriW
		0.0,
		0.0,
		0.0

	};
	*/


};


}
}
} // namespace

#endif