
#include "mdaDX10Processor.h"
#include "mdaDX10Controller.h"
#include <math.h>

#include "regReader.h"
#include <locale>
#include <codecvt>
#include "bcrypt/BCrypt.hpp"

//#include "C:\Users\PC\libbcrypt\include\bcrypt\BCrypt.hpp"


#define debugLog(...) fprintf(stderr, __VA_ARGS__)


namespace Steinberg {
namespace Vst {
namespace rs {

#define NOUTS    2       //number of outputs
#define SILENCE 0.00001f  //voice choking
	
	float DX10Processor::programParams[][GPARAMS] = {0.0f};
	
	

//-----------------------------------------------------------------------------
FUID DX10Processor::uid (0x3394EECF, 0x198E4B66, 0x80878031, 0xE531ADD6);

tresult PLUGIN_API DX10Processor::connect(IConnectionPoint* other) {
	// First connect normally
	tresult result = AudioEffect::connect(other);

	// Generate and share instance ID
	myInstanceId = reinterpret_cast<int64>(this);

	// Send ID to controller
	IMessage* msg = allocateMessage();
	if (msg) {
		msg->setMessageID("InstanceID");
		msg->getAttributes()->setInt("id", myInstanceId);
		sendMessage(msg);
		msg->release();
	}

	return result;
}

//-----------------------------------------------------------------------------
DX10Processor::DX10Processor () : currentProgram (0) {
	setControllerClass(DX10Controller::uid);
	std::string password = "4d01956a26c040343d6593d93bfe2e80";

	std::wstring regSubKey;
	regSubKey = L"Software\\Rogue-Syntax\\Registrations\\DonnaMedina\\";

	std::wstring regValue(L"RSRegKey");
	try{
		valueFromRegistry = GetStringValueFromHKLM(regSubKey, regValue);
	}
	catch (std::exception& e){
		regErr1 = e.what();
	}
	if (valueFromRegistry.size() > 0) {
		regKeyFound = true;
	}
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;
	std::string hashd = converter.to_bytes(valueFromRegistry);

	bool regIsValid = BCrypt::validatePassword(password, hashd);
	if (regIsValid == true ) {
		regKeyLegit = true;
		//used in DX10Processor::noteOn
		constants::isRegLegit = true;
	}
	else {
		regKeyLegit = false;
		constants::isRegLegit = false;
	}

	//regKeyLegit = true;
	//constants::isRegLegit = true;


	//Prima Donna #0
	programParams[1][OSC_] = 0.30f;
	programParams[1][MOD_OSC_] = 1.0f;
	programParams[1][WIDTH_] = 0.5f;
	programParams[1][WIDTH_ENV] = 0.5f;
	programParams[1][TUNE_ENV] = 0.5f;
	programParams[1][FINE_ENV] = 0.5f;
	programParams[1][OCT_ENV] = 0.5f;
	programParams[1][INT_ENV] = 0.5f;
	programParams[1][MOD_GAIN_ENV] = 0.28f;
	programParams[1][MOD_GAIN_ENV_SRC] = 1.0f;
	programParams[1][MOD_WIDTH_ENV] = 0.5f;
	programParams[1][MOD_TUNE_ENV] = 0.5f;
	programParams[1][MOD_FINE_ENV] = 0.5f;
	programParams[1][MOD_OCT_ENV] = 0.5f;
	programParams[1][MOD_INT_ENV] = 0.5f;
	programParams[1][MOD_BEAT_ENV] = 0.5f;
	programParams[1][MOD_OSC_GAIN] = 1.0f;
	programParams[1][MOD_BEAT_] = 0.0f;
	programParams[1][MOD_TYPE_] = 0.59f;
	programParams[1][MOD_WIDTH_] = 0.5f;
	programParams[1][TUNE_] = 0.5f;
	programParams[1][MOD_TUNE_] = 0.5f;
	programParams[1][FINE_] = 0.5f;
	programParams[1][MOD_FINE_] = 0.5f;
	programParams[1][OCT_] = 0.5f;
	programParams[1][INT_] = 0.014f;
	programParams[1][MOD_OCT_] = 0.46f;
	programParams[1][MOD_INT_] = 0.0f;
	programParams[1][GAIN_] = 0.75f;
	programParams[1][MOD_GAIN_] = 0.75f;
	programParams[1][GAIN_ENV] = 0.5f;
	programParams[1][FILTER_] = 0.92f;
	programParams[1][FILTER_Q] = 0.0f;
	programParams[1][FILTER_ENV] = 0.5f;
	programParams[1][PAN_LR] = 0.5f;
	programParams[1][PAN_LR_ENV] = 0.15f;
	programParams[1][PAN_LR_ENV_SRC] = 1.0f;
	programParams[1][ENV_1_DECAYTOGGLE] = 1.0f;
	programParams[1][ENV_2_DECAYTOGGLE] = 0.0f;
	programParams[1][ENV_3_DECAYTOGGLE] = 0.0f;
	programParams[1][LFO_1_BPM] = 0.0f;
	programParams[1][LFO_2_BPM] = 0.0f;
	programParams[1][LFO_3_BPM] = 0.0f;
	programParams[1][ENV_1_ATTACK] = 0.07;
	programParams[1][ENV_1_DECAY] = 0.87;
	programParams[1][ENV_1_RELEASE] = 0.87;
	programParams[1][ENV_1_DELAY] = 0.0;
	programParams[1][ENV_2_ATTACK] = 0.05;
	programParams[1][ENV_2_DECAY] = 0.55;
	programParams[1][ENV_2_RELEASE] = 1.0;
	programParams[1][ENV_2_DECAYTOGGLE] = 0.0;
	programParams[1][ENV_2_DELAY] = 0.0;
	programParams[1][ENV_3_ATTACK] = 0.11;
	programParams[1][ENV_3_DECAY] = 0.55;
	programParams[1][ENV_3_RELEASE] = 0.80;
	programParams[1][ENV_3_DECAYTOGGLE] = 0.0;
	programParams[1][ENV_3_DELAY] = 0.0;
	programParams[1][LFO_1_RATE] = 0.34f;
	programParams[1][LFO_2_RATE] = 0.53f;
	programParams[1][LFO_3_RATE] = 0.088f;
	programParams[1][LFO_1_AMP] = 0.14f;
	programParams[1][LFO_2_AMP] = 0.4f;
	programParams[1][LFO_3_AMP] = 0.4f;
	programParams[1][LFO_3_TYPE] = 0.54f;
	programParams[1][LFO_3_PHASE] = 0.08f;
	programParams[1][LFO_3_DELAY] = 0.38f;

	//softly #1
	programParams[0][OSC_] = 0.60f;//preset 1
	programParams[0][WIDTH_] = 0.5f;
	programParams[0][WIDTH_ENV] = 0.5f;
	programParams[0][TUNE_ENV] = 0.5f;
	programParams[0][FINE_ENV] = 0.5f;
	programParams[0][OCT_ENV] = 0.5f;
	programParams[0][INT_ENV] = 0.5f;
	programParams[0][MOD_OSC_GAIN] = 0.5f;
	programParams[0][MOD_GAIN_ENV] = 0.5f;
	programParams[0][MOD_WIDTH_ENV] = 0.5f;
	programParams[0][MOD_TUNE_ENV] = 0.5f;
	programParams[0][MOD_FINE_ENV] = 0.5f;
	programParams[0][MOD_OCT_ENV] = 0.5f;
	programParams[0][MOD_INT_ENV] = 0.5f;
	programParams[0][MOD_BEAT_ENV] = 0.5f;
	programParams[0][MOD_BEAT_] = 0.087f;
	programParams[0][MOD_TYPE_] = 1.0f;
	programParams[0][MOD_WIDTH_] = 0.5f;
	programParams[0][TUNE_] = 0.5f;
	programParams[0][MOD_TUNE_] = 0.5f;
	programParams[0][FINE_] = 0.5f;
	programParams[0][MOD_FINE_] = 0.5f;
	programParams[0][OCT_] = 0.5f;
	programParams[0][INT_] = 0.014f;
	programParams[0][MOD_OCT_] = 0.46f;
	programParams[0][MOD_INT_] = 0.0f;
	programParams[0][GAIN_] = 0.75f;
	programParams[0][MOD_GAIN_] = 0.3f;
	programParams[0][GAIN_ENV] = 0.5f;
	programParams[0][FILTER_] = 0.92f;
	programParams[0][FILTER_Q] = 0.3f;
	programParams[0][FILTER_ENV] = 0.5f;
	programParams[0][PAN_LR] = 0.5f;
	programParams[0][PAN_LR_ENV] = 0.5f;
	programParams[0][ENV_1_DECAYTOGGLE] = 0.0f;
	programParams[0][ENV_2_DECAYTOGGLE] = 0.0f;
	programParams[0][ENV_3_DECAYTOGGLE] = 0.0f;
	programParams[0][LFO_1_BPM] = 0.0f;
	programParams[0][LFO_2_BPM] = 0.0f;
	programParams[0][LFO_3_BPM] = 0.0f;
	programParams[0][ENV_1_ATTACK] = 0.05;
	programParams[0][ENV_1_DECAY] = 0.86;
	programParams[0][ENV_1_RELEASE] = 0.82;
	programParams[0][ENV_1_DECAYTOGGLE] = 0.0 ;
	programParams[0][ENV_1_DELAY] =  0.0 ;
	programParams[0][ENV_2_ATTACK] = 0.05;
	programParams[0][ENV_2_DECAY] = 0.82;
	programParams[0][ENV_2_RELEASE] = 0.82;
	programParams[0][ENV_2_DECAYTOGGLE] = 0.0;
	programParams[0][ENV_2_DELAY] = 0.0;
	programParams[0][ENV_3_ATTACK] = 0.05;
	programParams[0][ENV_3_DECAY] = 0.82;
	programParams[0][ENV_3_RELEASE] = 0.82;
	programParams[0][ENV_3_DECAYTOGGLE] = 0.0;
	programParams[0][ENV_3_DELAY] = 0.0;
	programParams[0][LFO_1_RATE] = 0.25f;
	programParams[0][LFO_2_RATE] = 0.20f;
	programParams[0][LFO_2_AMP] = 0.4f;
	programParams[0][MOD_GAIN_ENV] = 0.36f;
	programParams[0][MOD_GAIN_ENV_SRC] = 0.0f;
	programParams[0][LFO_3_RATE] = 0.25f;
	programParams[0][LFO_3_AMP] = 0.4f;
	programParams[0][LFO_3_PHASE] = 0.5f;
	programParams[0][LFO_3_DELAY] = 0.25f;
	programParams[0][PAN_LR_ENV] = 0.16f;
	programParams[0][PAN_LR_ENV_SRC] = 1.0f;


	//PERTRUB #2
	programParams[2][OSC_] = 0.90f;
	programParams[2][WIDTH_] = 0.5f;
	programParams[2][WIDTH_ENV] = 0.5f;
	programParams[2][TUNE_ENV] = 0.5f;
	programParams[2][FINE_ENV] = 0.5f;
	programParams[2][OCT_ENV] = 0.5f;
	programParams[2][INT_ENV] = 0.5f;
	programParams[2][MOD_OSC_GAIN] = 1.0f;
	programParams[2][MOD_OSC_] = 0.6f;
	programParams[2][MOD_GAIN_ENV] = 0.0f;
	programParams[2][MOD_GAIN_ENV_SRC] = 0.51f;
	programParams[2][MOD_WIDTH_ENV] = 0.5f;
	programParams[2][MOD_TUNE_ENV] = 0.5f;
	programParams[2][MOD_FINE_ENV] = 0.5f;
	programParams[2][MOD_OCT_ENV] = 0.5f;
	programParams[2][MOD_INT_ENV] = 0.5f;
	programParams[2][MOD_BEAT_ENV] = 0.5f;
	programParams[2][MOD_BEAT_] = 0.087f;
	programParams[2][MOD_TYPE_] = 1.0f;
	programParams[2][MOD_WIDTH_] = 0.5f;
	programParams[2][TUNE_] = 0.5f;
	programParams[2][MOD_TUNE_] = 0.5f;
	programParams[2][FINE_] = 0.5f;
	programParams[2][MOD_FINE_] = 0.5f;
	programParams[2][OCT_] = 0.5f;
	programParams[2][INT_] = 0.014f;
	programParams[2][MOD_OCT_] = 0.364f;
	programParams[2][MOD_INT_] = 0.63f;
	programParams[2][GAIN_] = 0.75f;
	programParams[2][MOD_GAIN_] = 0.5f;
	programParams[2][GAIN_ENV] = 0.5f;
	programParams[2][FILTER_] = 1.0f;
	programParams[2][FILTER_Q] = 0.35f;
	programParams[2][FILTER_ENV] = 0.0f;
	programParams[2][FILTER_ENV_SRC] = 0.99f;
	programParams[2][PAN_LR] = 0.5f;
	programParams[2][PAN_LR_ENV] = 0.0f;
	programParams[2][PAN_LR_ENV_SRC] = 0.66f;
	programParams[2][ENV_1_DECAYTOGGLE] = 0.0f;
	programParams[2][ENV_2_DECAYTOGGLE] = 0.0f;
	programParams[2][ENV_3_DECAYTOGGLE] = 0.0f;
	programParams[2][LFO_1_BPM] = 0.0f;
	programParams[2][LFO_2_BPM] = 0.0f;
	programParams[2][LFO_3_BPM] = 0.0f;
	programParams[2][ENV_1_ATTACK] = 0.05;
	programParams[2][ENV_1_DECAY] = 0.93;
	programParams[2][ENV_1_RELEASE] = 0.82;
	programParams[2][ENV_1_DECAYTOGGLE] = 0.0;
	programParams[2][ENV_1_DELAY] = 0.0;
	programParams[2][ENV_2_ATTACK] = 0.05;
	programParams[2][ENV_2_DECAY] = 0.82;
	programParams[2][ENV_2_RELEASE] = 0.82;
	programParams[2][ENV_2_DECAYTOGGLE] = 0.0;
	programParams[2][ENV_2_DELAY] = 0.0;
	programParams[2][ENV_3_ATTACK] = 0.5;
	programParams[2][ENV_3_DECAY] = 0.32;
	programParams[2][ENV_3_RELEASE] = 0.81;
	programParams[2][ENV_3_DECAYTOGGLE] = 0.0;
	programParams[2][ENV_3_DELAY] = 0.0;
	programParams[2][LFO_1_RATE] = 0.18f;
	programParams[2][LFO_1_AMP] = 0.5f;
	programParams[2][LFO_1_PHASE] = 0.5f;
	programParams[2][LFO_2_RATE] = 0.25f;
	programParams[2][LFO_2_AMP] = 0.61f;
	programParams[2][LFO_3_RATE] = 0.1968f;
	programParams[2][LFO_3_AMP] = 0.96f;
	programParams[2][LFO_3_PHASE] = 0.1691f;
	programParams[2][LFO_3_DELAY] = 0.036f;


	//polycrystaline #3
	programParams[3][OSC_] = 0.96f;
	programParams[3][MOD_OSC_] = 0.0f;
	programParams[3][WIDTH_] = 0.5f;
	programParams[3][WIDTH_ENV] = 0.5f;
	programParams[3][TUNE_ENV] = 0.5f;
	programParams[3][FINE_ENV] = 0.91f;
	programParams[3][FINE_ENV_SRC] = 0.99f;
	programParams[3][MOD_FINE_ENV] = 0.91f;
	programParams[3][MOD_FINE_ENV_SRC] = 0.99f;
	programParams[3][SYNC_GEN_MOD_TUNE] = 1.0f;
	programParams[3][OCT_ENV] = 0.5f;
	programParams[3][INT_ENV] = 0.5f;
	programParams[3][MOD_GAIN_ENV] = 0.5f;
	programParams[3][MOD_GAIN_ENV_SRC] = 0.0f;
	programParams[3][MOD_WIDTH_ENV] = 0.0f;
	programParams[3][MOD_WIDTH_ENV_SRC] = 1.0f;
	programParams[3][MOD_TUNE_ENV] = 0.5f;
	programParams[3][MOD_OCT_ENV] = 0.5f;
	programParams[3][MOD_INT_ENV] = 0.5f;
	programParams[3][MOD_BEAT_ENV] = 0.5f;
	programParams[3][MOD_OSC_GAIN] = 1.0f;
	programParams[3][MOD_BEAT_] = 0.0f;
	programParams[3][MOD_TYPE_] = 0.59f;
	programParams[3][MOD_WIDTH_] = 0.5f;
	programParams[3][TUNE_] = 0.5f;
	programParams[3][MOD_TUNE_] = 0.5f;
	programParams[3][FINE_] = 0.5f;
	programParams[3][MOD_FINE_] = 0.5f;
	programParams[3][OCT_] = 0.5f;
	programParams[3][INT_] = 0.014f;
	programParams[3][MOD_OCT_] = 0.46f;
	programParams[3][MOD_INT_] = 0.0f;
	programParams[3][GAIN_] = 0.75f;
	programParams[3][MOD_GAIN_] = 0.75f;
	programParams[3][GAIN_ENV] = 0.5f;
	programParams[3][FILTER_] = 0.92f;
	programParams[3][FILTER_Q] = 0.0f;
	programParams[3][FILTER_ENV] = 0.5f;
	programParams[3][PAN_LR] = 0.5f;
	programParams[3][PAN_LR_ENV] = 0.99f;
	programParams[3][PAN_LR_ENV_SRC] = 0.65f;
	programParams[3][ENV_1_DECAYTOGGLE] = 0.0f;
	programParams[3][ENV_2_DECAYTOGGLE] = 0.0f;
	programParams[3][ENV_3_DECAYTOGGLE] = 0.0f;
	programParams[3][LFO_1_BPM] = 0.0f;
	programParams[3][LFO_2_BPM] = 0.0f;
	programParams[3][LFO_3_BPM] = 0.0f;
	programParams[3][ENV_1_ATTACK] = 1.0f;
	programParams[3][ENV_1_DECAY] = 1.0f;
	programParams[3][ENV_1_RELEASE] = 1.0f;
	programParams[3][ENV_1_DELAY] = 0.0;
	programParams[3][ENV_2_ATTACK] = 0.05;
	programParams[3][ENV_2_DECAY] = 0.55;
	programParams[3][ENV_2_RELEASE] = 1.0;
	programParams[3][ENV_2_DECAYTOGGLE] = 0.0;
	programParams[3][ENV_2_DELAY] = 0.0;
	programParams[3][ENV_3_ATTACK] = 0.11;
	programParams[3][ENV_3_DECAY] = 0.55;
	programParams[3][ENV_3_RELEASE] = 0.80;
	programParams[3][ENV_3_DECAYTOGGLE] = 0.0;
	programParams[3][ENV_3_DELAY] = 0.0;
	programParams[3][LFO_1_RATE] = 0.23f;
	programParams[3][LFO_2_RATE] = 0.53f;
	programParams[3][LFO_3_RATE] = 0.088f;
	programParams[3][LFO_1_AMP] = 0.43f;
	programParams[3][LFO_2_AMP] = 0.4f;
	programParams[3][LFO_3_AMP] = 0.4f;
	programParams[3][LFO_3_TYPE] = 0.54f;
	programParams[3][LFO_1_PHASE] = 0.5f;
	programParams[3][LFO_3_PHASE] = 0.46f;
	programParams[3][LFO_3_DELAY] = 0.38f;

	//skillsaw #4
	programParams[4][OSC_] = 0.33f;
	programParams[4][MOD_OSC_] = 0.0f;
	programParams[4][WIDTH_] = 0.5f;
	programParams[4][WIDTH_ENV] = 0.5f;
	programParams[4][FINE_ENV] = 0.5f;
	programParams[4][FINE_ENV_SRC] = 0.0f;
	programParams[4][MOD_FINE_ENV] = 0.0f;
	programParams[4][MOD_FINE_ENV_SRC] = 0.0f;
	programParams[4][SYNC_GEN_MOD_TUNE] = 1.0f;
	programParams[4][OCT_ENV] = 0.5f;
	programParams[4][INT_ENV] = 0.5f;
	programParams[4][MOD_GAIN_ENV] = 0.5f;
	programParams[4][MOD_GAIN_ENV_SRC] = 0.0f;
	programParams[4][MOD_WIDTH_ENV] = 0.0f;
	programParams[4][MOD_WIDTH_ENV_SRC] = 1.0f;
	programParams[4][MOD_TUNE_ENV] = 1.0f;
	programParams[4][MOD_TUNE_ENV_SRC] = 0.68f;
	programParams[4][TUNE_ENV] = 1.0f;
	programParams[4][TUNE_ENV_SRC] = 0.68f;
	programParams[4][MOD_OCT_ENV] = 0.41f;
	programParams[4][MOD_OCT_ENV_SRC] = 0.52f;
	programParams[4][MOD_INT_ENV] = 0.5f;
	programParams[4][MOD_BEAT_ENV] = 0.5f;
	programParams[4][MOD_OSC_GAIN] = 0.72f;
	programParams[4][MOD_BEAT_] = 0.0f;
	programParams[4][MOD_TYPE_] = 0.65f;
	programParams[4][MOD_WIDTH_] = 0.5f;
	programParams[4][TUNE_] = 0.5f;
	programParams[4][MOD_TUNE_] = 0.5f;
	programParams[4][FINE_] = 0.5f;
	programParams[4][MOD_FINE_] = 0.5f;
	programParams[4][OCT_] = 0.5f;
	programParams[4][INT_] = 0.014f;
	programParams[4][MOD_OCT_] = 0.46f;
	programParams[4][MOD_INT_] = 0.0f;
	programParams[4][GAIN_] = 0.75f;
	programParams[4][MOD_GAIN_] = 0.75f;
	programParams[4][GAIN_ENV] = 0.5f;
	programParams[4][FILTER_] = 0.92f;
	programParams[4][FILTER_Q] = 0.2f;
	programParams[4][FILTER_ENV] = 0.5f;
	programParams[4][PAN_LR] = 0.5f;
	programParams[4][PAN_LR_ENV] = 0.15f;
	programParams[4][PAN_LR_ENV_SRC] = 1.0f;
	programParams[4][ENV_1_DECAYTOGGLE] = 1.0f;
	programParams[4][ENV_2_DECAYTOGGLE] = 0.0f;
	programParams[4][ENV_3_DECAYTOGGLE] = 1.0f;
	programParams[4][LFO_1_BPM] = 0.0f;
	programParams[4][LFO_2_BPM] = 0.0f;
	programParams[4][LFO_3_BPM] = 0.0f;
	programParams[4][ENV_1_ATTACK] = 0.0f;
	programParams[4][ENV_1_DECAY] = 1.0f;
	programParams[4][ENV_1_RELEASE] = 1.0f;
	programParams[4][ENV_1_DELAY] = 1.0;
	programParams[4][ENV_2_ATTACK] = 0.05;
	programParams[4][ENV_2_DECAY] = 0.55;
	programParams[4][ENV_2_RELEASE] = 1.0;
	programParams[4][ENV_2_DECAYTOGGLE] = 0.0;
	programParams[4][ENV_2_DELAY] = 0.0;
	programParams[4][ENV_3_ATTACK] = 0.0;
	programParams[4][ENV_3_DECAY] = 0.69;
	programParams[4][ENV_3_RELEASE] = 0.96;
	programParams[4][ENV_3_DECAYTOGGLE] = 0.0;
	programParams[4][ENV_3_DELAY] = 0.0;
	programParams[4][LFO_1_RATE] = 0.27f;
	programParams[4][LFO_2_RATE] = 0.52f;
	programParams[4][LFO_3_RATE] = 0.19f;
	programParams[4][LFO_1_AMP] = 0.24f;
	programParams[4][LFO_2_AMP] = 0.4f;
	programParams[4][LFO_3_AMP] = 0.4f;
	programParams[4][LFO_1_TYPE] = 0.43f;
	programParams[4][LFO_3_TYPE] = 0.00f;
	programParams[4][LFO_1_PHASE] = 0.52f;
	programParams[4][LFO_3_PHASE] = 0.5f;
	programParams[4][LFO_1_DELAY] = 0.28f;
	programParams[4][LFO_3_DELAY] = 0.37f;

	//brooding #5
	programParams[5][OSC_] = 0.66f;
	programParams[5][MOD_OSC_] = 1.0f;
	programParams[5][WIDTH_] = 0.27f;
	programParams[5][WIDTH_ENV] = 0.5f;
	programParams[5][TUNE_] = 0.5f;
	programParams[5][TUNE_ENV] = 0.5f;
	programParams[5][TUNE_ENV_SRC] = 0.0f;
	programParams[5][FINE_] = 0.5f;
	programParams[5][INT_] = 0.014f;
	programParams[5][INT_ENV] = 0.5f;
	programParams[5][FINE_ENV] = 0.8f;
	programParams[5][FINE_ENV_SRC] = 1.0f;
	programParams[5][OCT_] = 0.25f;
	programParams[5][OCT_ENV] = 0.5f;
	programParams[5][MOD_TYPE_] = 1.0f;
	programParams[5][MOD_OSC_GAIN] = 1.0f;
	programParams[5][MOD_GAIN_] = 0.6f;
	programParams[5][MOD_GAIN_ENV] = 1.0f;
	programParams[5][MOD_GAIN_ENV_SRC] = 1.0f;
	programParams[5][MOD_WIDTH_ENV] = 1.0f;
	programParams[5][MOD_WIDTH_ENV_SRC] = 1.0f;
	programParams[5][MOD_WIDTH_] = 0.54f;
	programParams[5][MOD_TUNE_] = 0.5f;
	programParams[5][MOD_TUNE_ENV] = 0.5f;
	programParams[5][MOD_TUNE_ENV_SRC] = 0.0f;
	programParams[5][MOD_FINE_] = 0.5f;
	programParams[5][MOD_FINE_ENV] = 0.8f;
	programParams[5][MOD_FINE_ENV_SRC] = 1.0f;
	programParams[5][MOD_OCT_] = 0.36f;
	programParams[5][MOD_INT_] = 0.0f;
	programParams[5][MOD_INT_ENV] = 0.04f;
	programParams[5][MOD_INT_ENV] = 0.48f;
	programParams[5][MOD_OCT_ENV] = 0.5f;
	programParams[5][MOD_OCT_ENV_SRC] = 0.0f;
	programParams[5][MOD_INT_ENV] = 0.5f;
	programParams[5][MOD_BEAT_ENV] = 0.5f;
	programParams[5][MOD_BEAT_] = 0.18f;
	programParams[5][GAIN_] = 0.75f;
	programParams[5][GAIN_ENV] = 0.5f;
	programParams[5][FILTER_] = 0.303f;
	programParams[5][FILTER_Q] = 0.27f;
	programParams[5][FILTER_ENV] = 1.0f;
	programParams[5][FILTER_ENV_SRC] = 1.0f;
	programParams[5][PAN_LR] = 0.51f;
	programParams[5][PAN_LR_ENV] = 0.26f;
	programParams[5][PAN_LR_ENV_SRC] = 0.64f;
	programParams[5][SYNC_GEN_MOD_TUNE] = 0.0f;
	programParams[5][ENV_1_DECAYTOGGLE] = 0.0f;
	programParams[5][ENV_2_DECAYTOGGLE] = 0.0f;
	programParams[5][ENV_3_DECAYTOGGLE] = 0.0f;
	programParams[5][LFO_1_BPM] = 0.0f;
	programParams[5][LFO_2_BPM] = 0.0f;
	programParams[5][LFO_3_BPM] = 0.0f;
	programParams[5][ENV_1_ATTACK] = 0.08f;
	programParams[5][ENV_1_DECAY] = 1.0f;
	programParams[5][ENV_1_RELEASE] = 0.65f;
	programParams[5][ENV_1_DELAY] = 1.0;
	programParams[5][ENV_2_ATTACK] = 0.0;
	programParams[5][ENV_2_DECAY] = 0.55;
	programParams[5][ENV_2_RELEASE] = 1.0;
	programParams[5][ENV_2_DECAYTOGGLE] = 0.0;
	programParams[5][ENV_2_DELAY] = 0.0;
	programParams[5][ENV_3_ATTACK] = 0.0;
	programParams[5][ENV_3_DECAY] = 0.30;
	programParams[5][ENV_3_RELEASE] = 0.8;
	programParams[5][ENV_3_DECAYTOGGLE] = 0.0;
	programParams[5][ENV_3_DELAY] = 0.0;
	programParams[5][LFO_1_RATE] = 0.15f;
	programParams[5][LFO_2_RATE] = 0.52f;
	programParams[5][LFO_3_RATE] = 0.16f;
	programParams[5][LFO_1_AMP] = 0.43f;
	programParams[5][LFO_2_AMP] = 0.40f;
	programParams[5][LFO_3_AMP] = 0.265f;
	programParams[5][LFO_1_TYPE] = 0.0f;
	programParams[5][LFO_2_TYPE] = 0.0f;
	programParams[5][LFO_3_TYPE] = 0.4f;
	programParams[5][LFO_1_PHASE] = 0.61f;
	programParams[5][LFO_3_PHASE] = 0.0f;
	programParams[5][LFO_1_DELAY] = 0.0f;
	programParams[5][LFO_3_DELAY] = 0.0f;

	//brooding2 #6
	programParams[6][OSC_] = 0.66f;
	programParams[6][MOD_OSC_] = 1.0f;
	programParams[6][WIDTH_] = 0.5f;
	programParams[6][WIDTH_ENV] = 0.5f;
	programParams[6][TUNE_] = 0.5f;
	programParams[6][TUNE_ENV] = 0.5f;
	programParams[6][TUNE_ENV_SRC] = 0.0f;
	programParams[6][FINE_] = 0.5f;
	programParams[6][INT_] = 0.014f;
	programParams[6][INT_ENV] = 0.5f;
	programParams[6][FINE_ENV] = 0.8f;
	programParams[6][FINE_ENV_SRC] = 1.0f;
	programParams[6][OCT_] = 0.0f;
	programParams[6][OCT_ENV] = 0.5f;
	programParams[6][MOD_TYPE_] = 1.0f;
	programParams[6][MOD_OSC_GAIN] = 1.0f;
	programParams[6][MOD_GAIN_] = 0.92f;
	programParams[6][MOD_GAIN_ENV] = 0.5f;
	programParams[6][MOD_GAIN_ENV_SRC] = 0.0f;
	programParams[6][MOD_WIDTH_ENV] = 0.4f;
	programParams[6][MOD_WIDTH_ENV_SRC] = 1.0f;
	programParams[6][MOD_WIDTH_] = 0.69f;
	programParams[6][MOD_TUNE_] = 0.5f;
	programParams[6][MOD_TUNE_ENV] = 0.5f;
	programParams[6][MOD_TUNE_ENV_SRC] = 0.0f;
	programParams[6][MOD_FINE_] = 0.5f;
	programParams[6][MOD_FINE_ENV] = 0.8f;
	programParams[6][MOD_FINE_ENV_SRC] = 1.0f;
	programParams[6][MOD_OCT_] = 0.36f;
	programParams[6][MOD_INT_] = 0.0f;
	programParams[6][MOD_OCT_ENV] = 0.5f;
	programParams[6][MOD_OCT_ENV_SRC] = 0.0f;
	programParams[6][MOD_INT_ENV] = 0.5f;
	programParams[6][MOD_BEAT_ENV] = 0.5f;
	programParams[6][MOD_BEAT_] = 0.45f;
	programParams[6][GAIN_] = 0.75f;
	programParams[6][GAIN_ENV] = 0.5f;
	programParams[6][FILTER_] = 0.19f;
	programParams[6][FILTER_Q] = 0.36f;
	programParams[6][FILTER_ENV] = 0.5f;
	programParams[6][FILTER_ENV_SRC] = 01.0f;
	programParams[6][PAN_LR] = 0.5f;
	programParams[6][PAN_LR_ENV] = 0.63f;
	programParams[6][PAN_LR_ENV_SRC] = 0.64f;
	programParams[6][SYNC_GEN_MOD_TUNE] = 0.0f;
	programParams[6][ENV_1_DECAYTOGGLE] = 0.0f;
	programParams[6][ENV_2_DECAYTOGGLE] = 0.0f;
	programParams[6][ENV_3_DECAYTOGGLE] = 0.0f;
	programParams[6][LFO_1_BPM] = 0.0f;
	programParams[6][LFO_2_BPM] = 0.0f;
	programParams[6][LFO_3_BPM] = 0.0f;
	programParams[6][ENV_1_ATTACK] = 0.74f;
	programParams[6][ENV_1_DECAY] = 1.0f;
	programParams[6][ENV_1_RELEASE] = 0.0f;
	programParams[6][ENV_1_DELAY] = 1.0;
	programParams[6][ENV_2_ATTACK] = 0.0;
	programParams[6][ENV_2_DECAY] = 0.55;
	programParams[6][ENV_2_RELEASE] = 1.0;
	programParams[6][ENV_2_DECAYTOGGLE] = 0.0;
	programParams[6][ENV_2_DELAY] = 0.0;
	programParams[6][ENV_3_ATTACK] = 0.1;
	programParams[6][ENV_3_DECAY] = 0.55;
	programParams[6][ENV_3_RELEASE] = 0.8;
	programParams[6][ENV_3_DECAYTOGGLE] = 0.0;
	programParams[6][ENV_3_DELAY] = 0.0;
	programParams[6][LFO_1_RATE] = 0.13f;
	programParams[6][LFO_2_RATE] = 0.52f;
	programParams[6][LFO_3_RATE] = 0.30f;
	programParams[6][LFO_1_AMP] = 0.43f;
	programParams[6][LFO_2_AMP] = 0.40f;
	programParams[6][LFO_3_AMP] = 0.11f;
	programParams[6][LFO_1_TYPE] = 0.0f;
	programParams[6][LFO_2_TYPE] = 0.0f;
	programParams[6][LFO_3_TYPE] = 0.6f;
	programParams[6][LFO_1_PHASE] = 0.5f;
	programParams[6][LFO_3_PHASE] = 0.5f;
	programParams[6][LFO_1_DELAY] = 0.0f;
	programParams[6][LFO_3_DELAY] = 0.0f;

	
	allocParameters (GPARAMS);

	for (int32 i = 0; i < GPARAMS; i++) {
		floatParams[i] = programParams[0][i];
	}
	
	rampedParams[MAINENV1_RAMPINDEX].level = mainEnv1Cenv;
	quarterSecond = sampleRate / 8;

	//setCurrentProgram(0);
	//sendPidVal(&PMSG_SET_PARAM_NRML_CHAR, MOD_BEAT_, 0.087);
}


//-----------------------------------------------------------------------------
DX10Processor::~DX10Processor (){

}

//-----------------------------------------------------------------------------
tresult PLUGIN_API DX10Processor::initialize (FUnknown* context)
{
	tresult res = BaseProcessor::initialize (context);
	if (res == kResultTrue){
		addEventInput (USTRING("MIDI in"), 1);
		addAudioOutput (USTRING("Stereo Out"), SpeakerArr::kStereo);

		notes[0] = EVENTS_DONE;
		sampleRate = (float)getSampleRate();
		ifs = 1.0 / sampleRate;

		recalculators[SYNC_ATTACK] = &DX10Processor::recalcSyncAttack;
		recalculators[SYNC_DEC] = &DX10Processor::recalcSyncDec;
		recalculators[SYNC_REL] = &DX10Processor::recalcSyncRel;
		recalculators[SYNC_TUNE] = &DX10Processor::recalcSyncTune;
		recalculators[SYNC_FINE] = &DX10Processor::recalcSyncFine;
		recalculators[SYNC_INT] = &DX10Processor::recalcSyncInt;
		recalculators[SYNC_MODGAIN] = &DX10Processor::recalcSyncModgain;
		recalculators[SYNC_MODOCT] = &DX10Processor::recalcSyncModOct;
		recalculators[SYNC_MODINT] = &DX10Processor::recalcSyncModInt;
		recalculators[SYNC_MODTUNE] = &DX10Processor::recalcSyncModTune;
		recalculators[SYNC_MODFINE] = &DX10Processor::recalcSyncModFine;
		recalculators[SYNC_GAIN] = &DX10Processor::recalcSyncGain;
		recalculators[SYNC_MODFREAQBEAT] = &DX10Processor::recalcSyncModBeat;

		recalculators[ENV_1_ATTACK] = &DX10Processor::recalcEnv1Attack;
		recalculators[ENV_1_DECAY] = &DX10Processor::recalcEnv1Decay;
		recalculators[ENV_1_RELEASE] = &DX10Processor::recalcEnv1Release;
		recalculators[ENV_1_DECAYTOGGLE] = &DX10Processor::recalcEnv1DecayToggle;
		recalculators[ENV_1_DELAY] = &DX10Processor::recalcEnv1Delay;

		recalculators[ENV_2_ATTACK] = &DX10Processor::recalcEnv2Attack;
		recalculators[ENV_2_DECAY] = &DX10Processor::recalcEnv2Decay;
		recalculators[ENV_2_RELEASE] = &DX10Processor::recalcEnv2Release;
		recalculators[ENV_2_DECAYTOGGLE] = &DX10Processor::recalcEnv2DecayToggle;
		recalculators[ENV_2_DELAY] = &DX10Processor::recalcEnv2Delay;

		recalculators[ENV_3_ATTACK] = &DX10Processor::recalcEnv3Attack;
		recalculators[ENV_3_DECAY] = &DX10Processor::recalcEnv3Decay;
		recalculators[ENV_3_RELEASE] = &DX10Processor::recalcEnv3Release;
		recalculators[ENV_3_DECAYTOGGLE] = &DX10Processor::recalcEnv3DecayToggle;
		recalculators[ENV_3_DELAY] = &DX10Processor::recalcEnv3Delay;

		recalculators[LFO_1_RATE] = &DX10Processor::recalcLfo1Rate;
		recalculators[LFO_1_AMP] = &DX10Processor::recalcLfo1Amp;
		recalculators[LFO_1_TYPE] = &DX10Processor::recalcLfo1Type;
		recalculators[LFO_1_BPM] = &DX10Processor::recalcLfo1BPM;
		recalculators[LFO_1_DELAY] = &DX10Processor::recalcLfo1Delay;

		recalculators[LFO_2_RATE] = &DX10Processor::recalcLfo2Rate;
		recalculators[LFO_2_AMP] = &DX10Processor::recalcLfo2Amp;
		recalculators[LFO_2_TYPE] = &DX10Processor::recalcLfo2Type;
		recalculators[LFO_2_BPM] = &DX10Processor::recalcLfo2BPM;
		recalculators[LFO_2_DELAY] = &DX10Processor::recalcLfo2Delay;

		recalculators[LFO_3_RATE] = &DX10Processor::recalcLfo3Rate;
		recalculators[LFO_3_AMP] = &DX10Processor::recalcLfo3Amp;
		recalculators[LFO_3_TYPE] = &DX10Processor::recalcLfo3Type;
		recalculators[LFO_3_BPM] = &DX10Processor::recalcLfo3BPM;
		recalculators[LFO_3_DELAY] = &DX10Processor::recalcLfo3Delay;

		recalculators[WIDTH_RESET] = &DX10Processor::recalcResetWidth;
		recalculators[TUNE_RESET] = &DX10Processor::recalcResetWidth;
		recalculators[FINE_RESET] = &DX10Processor::recalcResetWidth;

		recalculators[MOD_WIDTH_RESET] = &DX10Processor::recalcResetWidth;
		recalculators[MOD_TUNE_RESET] = &DX10Processor::recalcResetWidth;
		recalculators[MOD_FINE_RESET] = &DX10Processor::recalcResetWidth;

		recalculators[TOGGLE_GEN_VIEW] = &DX10Processor::recalcToggleGenView;

		recalculators[LFO_1_PHASE] = &DX10Processor::recalcLfo1Phase;
		recalculators[LFO_2_PHASE] = &DX10Processor::recalcLfo2Phase;
		recalculators[LFO_3_PHASE] = &DX10Processor::recalcLfo3Phase;

		recalculators[STEP_CTRL_TARG] = &DX10Processor::recalcStepCtrlTarg;

		recalculators[STEP_CTRL_1] = &DX10Processor::recalcStepCtrl1;
		recalculators[STEP_CTRL_2] = &DX10Processor::recalcStepCtrl2;
		recalculators[STEP_CTRL_3] = &DX10Processor::recalcStepCtrl3;
		recalculators[STEP_CTRL_4] = &DX10Processor::recalcStepCtrl4;
		recalculators[STEP_CTRL_5] = &DX10Processor::recalcStepCtrl5;
		recalculators[STEP_CTRL_6] = &DX10Processor::recalcStepCtrl6;
		recalculators[STEP_CTRL_7] = &DX10Processor::recalcStepCtrl7;
		recalculators[STEP_CTRL_8] = &DX10Processor::recalcStepCtrl8;
		recalculators[STEP_CTRL_9] = &DX10Processor::recalcStepCtrl9;
		recalculators[STEP_CTRL_10] = &DX10Processor::recalcStepCtrl10;
		recalculators[STEP_CTRL_11] = &DX10Processor::recalcStepCtrl11;
		recalculators[STEP_CTRL_12] = &DX10Processor::recalcStepCtrl12;


		sustain = 0;
		activevoices = 0;
		K = 0;
		activevoices = 0;

		for (int g = 0; g < GENCOUNT; g++) {
			generators[g].init();
			generators[g].parentProcessor = this;
			generators[g].sampleRate = (float)getSampleRate();
			generators[g].sampleRateDouble = getSampleRate();
			generators[g].paramKeys = &paramKeys;
			generators[g].genIdx = g;
			generators[g].paramStartIdx = (PARTPARAMS * generators[g].genIdx);
			for (int i = 0; i <NPARAMS; i++) {
				generators[g].partParams[i] = &floatParams[i];
			}
			generators[g].silenceCutoff = SILENCE;
			generators[g].sustainNote = SUSTAIN;
			generators[g].createVoice();
			generators[g].filterEnv = 0.0f;
			generators[g].lfo0 = 0.0f;
			generators[g].dlfo = 0.0f;
			generators[g].modwhl = 0.0f;
			generators[g].lfo1 = 1.0f;
			generators[g].pbend = 1.0f;					
			generators[g].lfo0 = generators[g].dlfo = generators[g].modwhl = 0.0f;
			generators[g].lfo1 = generators[g].pbend = 1.0f;
			generators[g].volume = 0.0065f;
			generators[g].sustain = generators[g].activevoices = 0;
			generators[g].K = 0;
			//generators[g].queueTimer = Timer::create(&generators[g], 15);
			generators[g].E1catt = 0.0f;
			generators[g].E1cdec = 0.5f;
			generators[g].E1cdecay = 0.5f;
			generators[g].E1crel = 0.5f;
			generators[g].x = 0.0f;			
			generators[g].xLev = 0.0f;
			generators[g].swLev = 0.0f;
			generators[g].sinLev = 0.0f;
			generators[g].sinLevInc = 0.0f;
			generators[g].xLevInc = 0.0f;
			generators[g].swLevInc = 0.0f;
			generators[g].sinLevTarg = 0.0f;
			generators[g].xLevTarg = 0.0f;
			generators[g].swLevTarg = 0.0f;
			generators[g].modOneTarg = 0.0f;
			generators[g].modOneInc = 0.0f;
			generators[g].modOne = 0.0f;
			generators[g].modGainTarg = 0.0f;
			generators[g].modGainInc = 0.0f;
			generators[g].modGainLevel = 0.0f;

			generators[g].WTTables.setSampleRate(&generators[g].sampleRateDouble);
			generators[g].WTTables.m_uWaveform = generators[g].WTTables.SAW1;

			generators[g].halfSecSamples = (sampleRate / 2);

			generators[g].mainCEnvPtr = &mainEnv1Cenv;

			for (int i = 0; i < 7; i++) {
				generators[g].EnvLfoCtrls[i] = &EnvLfoCtrls[i];
			}
		}
		/*
		generators[0].ctrlValues[OSC_] = 2;
		generators[0].ctrlValues[OCT_] = 3;
		generators[0].ctrlValues[INT_] = 1;//these make no diff
		generators[0].ctrlValues[MOD_OCT_] = 6;
		generators[0].ctrlValues[MOD_INT_] = 1;
		generators[0].ctrlValues[MOD_BEAT_] = 2;
		generators[0].ctrlValues[MOD_TYPE_] = 3;
		generators[0].ctrlValues[PAN_LR_ENV_SRC] = 6;
		generators[0].ctrlValues[MOD_GAIN_ENV_SRC] = 5;
		*/
		recalculate ();
		setCurrentProgram(0);
	}



	return res;
}


tresult PLUGIN_API DX10Processor::notify(IMessage* message) {
	BaseProcessor::notify(message);
	msgInt = (*message->getMessageID());

	int64 myInstanceId_ = -1;
	message->getAttributes()->getInt("id", myInstanceId_);

	if (this->myInstanceId != myInstanceId && myInstanceId_ != -1 ) {
		return kResultFalse;
	}

	int64 pid;
	switch (msgInt) {
	case PMSG_STEP_SWITCH_CHAR:
		message->getAttributes()->getInt("0", STEP_SWITCH_VAL);
		message->getAttributes()->getInt("1", pid);
		//int g = (int)(pid / PARTPARAMS);
		generators[0].ctrlValues[pid] = STEP_SWITCH_VAL;
		generators[0].controls[pid] = static_cast<float>(STEP_SWITCH_VAL);
		return kResultTrue;
		break;

	case PMSG_LFO_RATE_CHAR:
		message->getAttributes()->getFloat("0", LFO_RATE_VAL);
		message->getAttributes()->getInt("1", pid);
		(this->*recalculators[pid])(LFO_RATE_VAL);

		return kResultTrue;
		break;

	case PMSG_CTRLR_READY_CHAR:
		//setCurrentProgram(0);

		return kResultTrue;
		break;

	}
	return ComponentBase::notify(message);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API DX10Processor::terminate ()
{
	return BaseProcessor::terminate ();
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API DX10Processor::setActive (TBool state){
	if (state) {
		for (genCountIdx = 0; genCountIdx < GENCOUNT; genCountIdx++) {
			generators[genCountIdx].setActive();
		}




		//for (int i = 0; i < PARTPARAMS; i++) {
		//	generators[0].recalc2(i);
		//}
		//for vertSwitchPids[_GEN_STEP_PARAM_COUNT_] send message to changeVertSwitch?
		//seems there should be a better way is there a similar set program method in ctrolers?

		//for (int i = ENV_1_ATTACK; i < STEP_CTRL_TARG + 1; i++) {//DO WE NEED THIS// initializes levels
		//	(this->*recalculators[i])(floatParams[i]);
			//sendPidVal(&PMSG_STEP_SWITCH2_CHAR, i, floatParams[i]);
		//}
		//message->getAttributes()->getInt("0", pid);
		//message->getAttributes()->getFloat("1", value);

		for (int i = 0; i < _GEN_STEP_PARAM_COUNT_; i++) {
			loadPID = vertSwitchPids[i];
			loadVal = floatParams[loadPID];
			//sendPidVal(&PMSG_STEP_SWITCH2_CHAR, loadPID, loadVal);
			sendPidVal(&PMSG_STEP_SWITCH2_CHAR, loadPID, loadVal);
		}

		//this is where we're finally ready to set the preset
		//setCurrentProgram(0);
		//this interferes with loading saved states!
	}
	return BaseProcessor::setActive (state);

}

//-----------------------------------------------------------------------------
void DX10Processor::setParameter (ParamID index, ParamValue newValue, int32 sampleOffset){
	//desnt seem to be doing 
	// WHAT IS THIS DOING
	//BASIC CONTROLLS
	if (index < GPARAMS) {
		floatParams[index] = newValue;
	}

	else if (index == BaseController::kPresetParam){ // program change
		currentProgram = std::min<int32> (kNumPrograms - 1, (int32)(newValue * kNumPrograms));

		const float* newParams = programParams[currentProgram];

		if (newParams) {
			for (int32 i = 0; i < GPARAMS; i++) {
				floatParams[i] = newParams[i];
			}	
		}

	} else if (index == BaseController::kModWheelParam){ // mod wheel
		newValue *= 127.;
		generators[0].modwhl = 0.00000005f * (float)(newValue * newValue);
	}
	else if (index == BaseController::kPitchBendParam){ // pitch bend
		if (newValue <= 1){
			generators[0].pbend = (newValue - 0.5) * 0x2000;
		} else{
			generators[0].pbend = newValue;	
		}

		if (generators[0].pbend > 0.0f) {
			generators[0].pbend = 1.0f + 0.000014951f * generators[0].pbend;
		} else {
			generators[0].pbend = 1.0f + 0.000013318f * generators[0].pbend;
		}
	}
}

//-----------------------------------------------------------------------------
void DX10Processor::setCurrentProgram(Steinberg::uint32 val) {
	currentProgram = val;

	for (int32 i = 0; i < GPARAMS; i++) {
		//setParameter(i, programParams[val][i], 0);
		floatParams[i] = programParams[val][i];
		//generators[0].partParams[i] = &floatParams[i];
	}

	for (int i = 0; i < PARTPARAMS; i++) {
		generators[0].recalc2(i);
	}
	//for vertSwitchPids[_GEN_STEP_PARAM_COUNT_] send message to changeVertSwitch?
	//seems there should be a better way is there a similar set program method in ctrolers?

	for (int i = ENV_1_ATTACK; i < STEP_CTRL_TARG + 1; i++) {//DO WE NEED THIS// initializes levels
		(this->*recalculators[i])(floatParams[i]);
		sendPidVal(&PMSG_STEP_SWITCH2_CHAR, i, floatParams[i]);
	}
	//message->getAttributes()->getInt("0", pid);
	//message->getAttributes()->getFloat("1", value);
	
	for (int i = 0; i < _GEN_STEP_PARAM_COUNT_; i++) {
		loadPID = vertSwitchPids[i];
		loadVal = floatParams[loadPID];
		//sendPidVal(&PMSG_STEP_SWITCH2_CHAR, loadPID, loadVal);
		sendPidVal(&PMSG_STEP_SWITCH2_CHAR, loadPID, loadVal);
	}
	for (int i = 0; i < TOGGLE_COUNT; i++) {
		loadPID = toggleBtnPids[i];
		loadVal = floatParams[loadPID];
		//sendPidVal(&PMSG_STEP_SWITCH2_CHAR, loadPID, loadVal);
		sendPidVal(&PMSG_STEP_SWITCH2_CHAR, loadPID, loadVal);
	}

	for (int i = 0; i < GENRAMPEDPARAMSLENGTH; i++) {
		generators[0].rampedParams[i].initLevel(floatParams[generators[0].rampedList[i]]);
	}

}
//-----------------------------------------------------------------------------
void DX10Processor::setCurrentProgramNormalized (ParamValue val) {
	setCurrentProgram (std::min<int32> (kNumPrograms - 1, (int32)(val * kNumPrograms)));
}


void DX10Processor::preProcess(ProcessData& data)
{
	// Handle parameter changes first
	if (processParameterChanges(data.inputParameterChanges))
		recalculate();

	// Pre-process events and organize them by sample position
	if (data.inputEvents)
		processEvents(data.inputEvents);

	// If we have samples to process and we're not bypassed
	if (data.numSamples > 0 && !bypassProcessing(data))
	{
		// Debug: Print out events for this block
		int32 debugIdx = 0;
		while (notes[debugIdx] != EVENTS_DONE) {
			//debugLog("Event at sample %d: note=%d, vel=%d\n", notes[debugIdx], notes[debugIdx + 1], notes[debugIdx + 2]);
			debugIdx += 3;
		}

		// Process audio in chunks divided by event boundaries
		int32 currentSample = 0;
		int32 eventIndex = 0;

		while (currentSample < data.numSamples) {
			// Find next event position
			int32 nextEventPos = data.numSamples; // Default to end of buffer

			if (notes[eventIndex] != EVENTS_DONE) {
				nextEventPos = notes[eventIndex]; // Get next event position

				// Make sure we don't process beyond the buffer
				if (nextEventPos > data.numSamples)
					nextEventPos = data.numSamples;
				else if (nextEventPos < currentSample) {
					// This could happen if events are improperly sorted or if there's an off-by-one error
					// Skip this event and move to the next one
					//debugLog("Warning: Event position (%d) is before current sample (%d)\n", nextEventPos, currentSample);
					eventIndex += 3;
					continue;
				}
			}

			// Calculate how many samples to process until next event
			int32 samplesToProcess = nextEventPos - currentSample;

			if (samplesToProcess > 0) {
				// Create a sub-ProcessData for this chunk
				ProcessData chunkData = data;
				chunkData.numSamples = samplesToProcess;

				// Adjust output buffer pointers for this chunk
				for (int32 i = 0; i < data.outputs[0].numChannels; i++) {
					chunkData.outputs[0].channelBuffers32[i] =
						data.outputs[0].channelBuffers32[i] + currentSample;
				}

				// Process this chunk
				doProcessing(chunkData);

				// Update current position
				currentSample += samplesToProcess;
			}

			// Process all events at the current position
			bool processedEvents = false;
			while (notes[eventIndex] != EVENTS_DONE && notes[eventIndex] <= currentSample) {
				// Handle the event
				int32 note = notes[eventIndex + 1];
				int32 velocity = notes[eventIndex + 2];

				//debugLog("Processing event: sample=%d, note=%d, vel=%d, currentSample=%d\n", notes[eventIndex], note, velocity, currentSample);

				// Call noteOn to handle both note-on and note-off
				// In a more robust implementation, we'd add a noteMap to track note->noteID mapping
				noteOn(note, velocity, noteID);
				processedEvents = true;

				// Update noteID for note-on events
				if (velocity > 0) {
					// Store the mapping of note to noteID
					noteToIDMap[note] = noteID;

					if (noteID < 126)
						noteID++;
					else
						noteID = 0;
				}
				else {
					// For note-off, try to use the same noteID as the note-on if possible
					// This needs a map from MIDI note number to noteID
					auto it = noteToIDMap.find(note);
					if (it != noteToIDMap.end()) {
						// We found the noteID that was used for this note
						noteToIDMap.erase(it);
					}
				}

				// Move to next event
				eventIndex += 3;
			}

			// If we processed events and we still have more samples left in the buffer,
			// we should continue to the next chunk
			if (processedEvents && currentSample < data.numSamples) {
				continue;
			}
		}

		// Check if output is silent
		checkSilence(data);
	}
}

//-----------------------------------------------------------------------------
/*
void DX10Processor::doProcessing (ProcessData& data) {
	o = 0.0;
	sampleFrames = data.numSamples;
	out1 = data.outputs[0].channelBuffers32[0];
	out2 = data.outputs[0].channelBuffers32[1];
	event = 0;
	frame = 0;
	bpm = data.processContext->tempo;

	if ( activevoices > 0 || notes[event] < sampleFrames ){ //detect & bypass completely empty blocks
		
		while (frame<sampleFrames){
			frames = notes[event++];
			if (frames > sampleFrames) {
				frames = sampleFrames;
			}
			frames -= frame;
			frame += frames;
			while (--frames >= 0) {

				generators[0].mainProcessing();
				*out1++ = generators[0].o1;
				*out2++ = generators[0].o2;

			}

			if (frame<sampleFrames){//next note on/off
				int32 note = notes[event++];
				int32 vel = notes[event++];
				noteOn(note, vel, noteID);//add a note id here
				if (noteID < 126) {
					noteID = noteID+1;
				}
				else {
					noteID = 0;
				}
			}
		}
		
		activevoices = 0;
		
		generators[0].bpm = data.processContext->tempo;
		if (EnvLfoCtrls[4].BPM != bpm) {
			recalcLfo1Rate(EnvLfoCtrls[4].rateValue);
		}
		if (EnvLfoCtrls[5].BPM != bpm) {
			recalcLfo2Rate(EnvLfoCtrls[5].rateValue);
		}
		if (EnvLfoCtrls[6].BPM != bpm) {
			recalcLfo3Rate(EnvLfoCtrls[6].rateValue);
		}

		generators[0].detectSilence(&activevoices);
		
		
	} else { //completely empty block
		while ( --sampleFrames >= 0) {
			*out1++ = 0.0f;
			*out2++ = 0.0f;
		}
		data.outputs[0].silenceFlags = 3;
	}
	notes[0] = EVENTS_DONE;
	//data.processContext->barPositionMusic
}
*/

void DX10Processor::doProcessing(ProcessData& data) {
	sampleFrames = data.numSamples;
	out1 = data.outputs[0].channelBuffers32[0];
	out2 = data.outputs[0].channelBuffers32[1];
	bpm = data.processContext->tempo;

	// Check for active voices or just process the chunk
	//if (activevoices > 0) {
		// Process all samples in this chunk
		for (int32 i = 0; i < sampleFrames; i++) {
			// Generate audio for all active generators
			generators[0].mainProcessing();
			*out1++ = generators[0].o1;
			*out2++ = generators[0].o2;
		}

		// Update BPM-dependent parameters if needed
		generators[0].bpm = bpm;
		if (EnvLfoCtrls[4].BPM != bpm) {
			recalcLfo1Rate(EnvLfoCtrls[4].rateValue);
		}
		if (EnvLfoCtrls[5].BPM != bpm) {
			recalcLfo2Rate(EnvLfoCtrls[5].rateValue);
		}
		if (EnvLfoCtrls[6].BPM != bpm) {
			recalcLfo3Rate(EnvLfoCtrls[6].rateValue);
		}

		// Check if we've gone silent
		//generators[0].detectSilence(&activevoices);
	/* }
	else {
		// Empty block, output silence
		for (int32 i = 0; i < sampleFrames; i++) {
			*out1++ = 0.0f;
			*out2++ = 0.0f;
		}
		data.outputs[0].silenceFlags = 3;
	}
	*/
}

//-----------------------------------------------------------------------------
/*
void DX10Processor::processEvents (IEventList* events)
{
	if (events) {
		int32 npos=0;
		int32 count = events->getEventCount ();
		for (int32 i = 0; i < count; i++) {
			Event e;
			events->getEvent (i, e);
			switch (e.type) {
				case Event::kNoteOnEvent:{

						notes[npos++] = e.sampleOffset;
						notes[npos++] = e.noteOn.pitch;
						notes[npos++] = e.noteOn.velocity * 127;
					
					
					break;
				}
				case Event::kNoteOffEvent:{
					notes[npos++] = e.sampleOffset;
					notes[npos++] = e.noteOff.pitch;
					notes[npos++] = 0;
					break;
				}
				default:
					continue;
			}
			if (npos > EVENTBUFFER) npos -= 3; //discard events if buffer full!!
		}
		notes[npos] = EVENTS_DONE;
	}
}
*/
void DX10Processor::processEvents(IEventList* events)
{
	if (events) {
		int32 npos = 0;
		int32 count = events->getEventCount();

		// First pass: count required buffer size
		int32 requiredSize = 0;
		for (int32 i = 0; i < count; i++) {
			Event e;
			events->getEvent(i, e);
			if (e.type == Event::kNoteOnEvent || e.type == Event::kNoteOffEvent) {
				requiredSize += 3; // Each note event requires 3 slots
			}
		}

		// Check if buffer will overflow
		if (requiredSize > EVENTBUFFER - 1) { // -1 for EVENTS_DONE marker
			// Priority strategy: keep note-offs and most recent note-ons
			int32 noteOffs = 0;

			// First, count note-offs
			for (int32 i = 0; i < count; i++) {
				Event e;
				events->getEvent(i, e);
				if (e.type == Event::kNoteOffEvent) {
					noteOffs += 3;
				}
			}

			// Process note-offs first (to prevent stuck notes)
			for (int32 i = 0; i < count; i++) {
				Event e;
				events->getEvent(i, e);
				if (e.type == Event::kNoteOffEvent && npos < EVENTBUFFER - 1) {
					notes[npos++] = e.sampleOffset;
					notes[npos++] = e.noteOff.pitch;
					notes[npos++] = 0; // velocity for note-off
				}
			}

			// Then process note-ons with remaining space
			// Start from most recent note-ons (reverse order)
			for (int32 i = count - 1; i >= 0; i--) {
				Event e;
				events->getEvent(i, e);
				if (e.type == Event::kNoteOnEvent && npos < EVENTBUFFER - 1) {
					notes[npos++] = e.sampleOffset;
					notes[npos++] = e.noteOn.pitch;
					notes[npos++] = e.noteOn.velocity * 127;
				}
			}
		}
		else {
			// Normal case: process all events in order
			for (int32 i = 0; i < count; i++) {
				Event e;
				events->getEvent(i, e);

				switch (e.type) {
				case Event::kNoteOnEvent: {
					if (npos <= EVENTBUFFER - 3) {
						notes[npos++] = e.sampleOffset;
						notes[npos++] = e.noteOn.pitch;
						notes[npos++] = e.noteOn.velocity * 127;
					}
					break;
				}
				case Event::kNoteOffEvent: {
					if (npos <= EVENTBUFFER - 3) {
						notes[npos++] = e.sampleOffset;
						notes[npos++] = e.noteOff.pitch;
						notes[npos++] = 0;
					}
					break;
				}
				default:
					continue;
				}
			}
		}

		// Sort events by sample offset to maintain proper chronological order
		sortEventsByOffset(notes, npos);

		// Mark the end of events
		notes[npos] = EVENTS_DONE;
	}
}

// Helper function to sort events by sample offset
void DX10Processor::sortEventsByOffset(int32* notes, int32 npos)
{
	// First sort by sample offset
	for (int i = 0; i < npos; i += 3) {
		for (int j = i + 3; j < npos; j += 3) {
			// Compare sample offsets
			if (notes[i] > notes[j]) {
				// Swap the events
				for (int k = 0; k < 3; k++) {
					int32 temp = notes[i + k];
					notes[i + k] = notes[j + k];
					notes[j + k] = temp;
				}
			}
			// For events at the same sample offset, make sure note-offs come after note-ons
			else if (notes[i] == notes[j]) {
				// notes[i+2] is velocity for first event
				// notes[j+2] is velocity for second event
				// If first is a note-off (vel=0) and second is a note-on (vel>0), swap them
				if (notes[i + 2] == 0 && notes[j + 2] > 0) {
					for (int k = 0; k < 3; k++) {
						int32 temp = notes[i + k];
						notes[i + k] = notes[j + k];
						notes[j + k] = temp;
					}
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------
/*
void DX10Processor::noteOn(int32 note, int32 velocity, int32 noteID){
	if (regKeyLegit == true) {
		
		if (velocity > 0) { //note on
			if (mainEnv1Env > 0.0001) {
				processNewMainEnv();
			}
			else {
				processNewMainEnv();
			}

			for (int i = 0; i < GENCOUNT; i++) {
				generators[i].noteOn(&note, &velocity, &noteID);
			}

		}
		else { //note off

			if (mainEnv1ReleaseToggle < 0.5) {
				mainEnv1Dec = mainEnv1Rel;
			}
			if (mainEnv1Dec == 1.0) {
				mainEnv1Dec = 0.98f;
			}

			mainEnv1Env = mainEnv1Cenv;
			mainEnv1Att = 1.0f;

			for (int i = 0; i < GENCOUNT; i++) {
				generators[i].noteOff(&note, &noteID);
			}

		}
	}
	
	
}
*/

void DX10Processor::noteOn(int32 note, int32 velocity, int32 noteID) {
	if (regKeyLegit == true) {

		if (velocity > 0) { // note on
			if (mainEnv1Env > 0.0001) {
				processNewMainEnv();
			}
			else {
				processNewMainEnv();
			}
			for (int i = 0; i < GENCOUNT; i++) {
				generators[i].noteOn(&note, &velocity, &noteID);
			}

			// Debug
			//debugLog("Note ON: note=%d, vel=%d, noteID=%d\n", note, velocity, noteID);
		}
		else { // note off
			if (mainEnv1ReleaseToggle < 0.5) {
				mainEnv1Dec = mainEnv1Rel;
			}
			if (mainEnv1Dec == 1.0) {
				mainEnv1Dec = 0.98f;
			}
			mainEnv1Env = mainEnv1Cenv;
			mainEnv1Att = 1.0f;

			// Look up the noteID from the map if available
			auto it = noteToIDMap.find(note);
			int32 noteIDToUse = noteID; // Default
			if (it != noteToIDMap.end()) {
				noteIDToUse = it->second;
				//debugLog("Using stored noteID %d for note-off of note %d\n", noteIDToUse, note);
			}

			for (int i = 0; i < GENCOUNT; i++) {
				generators[i].noteOff(&note, &noteIDToUse);
			}

			// Debug
			//debugLog("Note OFF: note=%d, noteID=%d\n", note, noteIDToUse);
		}
	}
}
void DX10Processor::retriggerMainEnv() {

}

void DX10Processor::processNewMainEnv() {
	
}

void DX10Processor::mainEnvProcessing() {

};


//CONTROLRECALS

int DX10Processor::recalcSyncAttack(double value) { return 0; };

int DX10Processor::recalcSyncDec(double value) { return 0; };

int DX10Processor::recalcSyncRel(double value) { return 0; };

int DX10Processor::recalcSyncTune(double value) { return 0; };

int DX10Processor::recalcSyncFine(double value) { return 0; };

int DX10Processor::recalcSyncInt(double value) { return 0; };

int DX10Processor::recalcSyncModgain(double value) { return 0; };

int DX10Processor::recalcSyncModOct(double value) { return 0; };

int DX10Processor::recalcSyncModInt(double value) { return 0; };

int DX10Processor::recalcSyncModTune(double value) { return 0; };

int DX10Processor::recalcSyncModFine(double value) { return 0; };

int DX10Processor::recalcSyncGain(double value) { return 0; };

int DX10Processor::recalcSyncModBeat(double value) { return 0; };



int DX10Processor::recalcEnv1Attack(double value) { 

	EnvLfoCtrls[1].catt = 1.0f - (float)exp(-ifs * exp(8.0 - 8.0 * value));

	return 0; 
};

int DX10Processor::recalcEnv1Decay(double value) { 
	EnvLfoCtrls[1].cdecay = (value);
	if (EnvLfoCtrls[1].cdecay > 0.991f) {
		EnvLfoCtrls[1].cdec = 1.0f;
	}
	else {
		EnvLfoCtrls[1].cdec = (float)exp(-ifs * exp(6.0 - 6.0 * EnvLfoCtrls[1].cdecay));
	}
	
	return 0; 
};


int DX10Processor::recalcEnv1Release(double value) { 
	EnvLfoCtrls[1].crel = (float)exp(-ifs * exp(5.0 - 5.0 * (value)));
	return 0; 
};

int DX10Processor::recalcEnv1DecayToggle(double value) { 
	EnvLfoCtrls[1].decayReleaseToggle = value;
	return 0;
};

int DX10Processor::recalcEnv1Delay(double value) { 
	EnvLfoCtrls[1].lfoDelay = value;
	return 0;
};



int DX10Processor::recalcEnv2Attack(double value) {

	EnvLfoCtrls[2].catt = 1.0f - (float)exp(-ifs * exp(8.0 - 8.0 * value));

	return 0;
};

int DX10Processor::recalcEnv2Decay(double value) {
	EnvLfoCtrls[2].cdecay = (value);
	if (EnvLfoCtrls[2].cdecay > 0.991f) {
		EnvLfoCtrls[2].cdec = 1.0f;
	}
	else {
		EnvLfoCtrls[2].cdec = (float)exp(-ifs * exp(6.0 - 6.0 * EnvLfoCtrls[2].cdecay));
	}

	return 0;
};


int DX10Processor::recalcEnv2Release(double value) {
	EnvLfoCtrls[2].crel = (float)exp(-ifs * exp(5.0 - 5.0 * (value)));
	return 0;
};

int DX10Processor::recalcEnv2DecayToggle(double value) {
	EnvLfoCtrls[2].decayReleaseToggle = value;
	return 0;
};

int DX10Processor::recalcEnv2Delay(double value) {
	EnvLfoCtrls[2].lfoDelay = value;
	return 0;
};



int DX10Processor::recalcEnv3Attack(double value) {

	EnvLfoCtrls[3].catt = 1.0f - (float)exp(-ifs * exp(8.0 - 8.0 * value));

	return 0;
};

int DX10Processor::recalcEnv3Decay(double value) {
	EnvLfoCtrls[3].cdecay = (value);
	//EnvLfoCtrls[3].cdec = (float)exp(-ifs * exp(8.0 - 8.0 * EnvLfoCtrls[3].cdecay));
	if (EnvLfoCtrls[3].cdecay > 0.991f) {
		EnvLfoCtrls[3].cdec = 1.0f;
	}
	else {
		EnvLfoCtrls[3].cdec = (float)exp(-ifs * exp(6.0 - 6.0 * EnvLfoCtrls[3].cdecay));
	}

	return 0;
};


int DX10Processor::recalcEnv3Release(double value) {
	EnvLfoCtrls[3].crel = (float)exp(-ifs * exp(5.0 - 5.0 * (value)));
	return 0;
};

int DX10Processor::recalcEnv3DecayToggle(double value) {
	EnvLfoCtrls[3].decayReleaseToggle = value;
	return 0;
};

int DX10Processor::recalcEnv3Delay(double value) {
	EnvLfoCtrls[3].lfoDelay = value;
	return 0;
};



int DX10Processor::recalcLfo1Rate(double value) { 
	EnvLfoCtrls[4].rateValue = value;
	EnvLfoCtrls[4].BPM = bpm;
	if (EnvLfoCtrls[4].syncBpm == 0) {
		EnvLfoCtrls[4].rate = cookVSTGUIVariable(MIN_LFO_RATE, MAX_LFO_RATE, value);
	
	}
	else {
		
		quantDivisor = static_cast<int>(value * 10);
		EnvLfoCtrls[4].rate = bpm / quants[quantDivisor];

	}
	sendDoubleAttr(&PMSG_LFO4_RATE_CHAR, "0", EnvLfoCtrls[4].rate);
	mainEnvLfo[4].m_dOscFo = EnvLfoCtrls[4].rate;

	return 0; 
};
int DX10Processor::recalcLfo1Amp(double value) {
	EnvLfoCtrls[4].level = value;
	mainEnvLfo[4].m_dAmplitude = value;
	return 0;
}
int DX10Processor::recalcLfo1Type(double value) { 
	EnvLfoCtrls[4].lfoType = static_cast<int>(div((value * 100), 25).quot);
	return 0; 
};

int DX10Processor::recalcLfo1BPM(double value) { 
	EnvLfoCtrls[4].syncBpm = (int)value;
	return 0; 
};

int DX10Processor::recalcLfo1Delay(double value) {
	EnvLfoCtrls[4].lfoDelay = value;
	return 0;
};

int DX10Processor::recalcLfo2Rate(double value) {
	EnvLfoCtrls[5].rateValue = value;
	EnvLfoCtrls[5].BPM = bpm;
	if (EnvLfoCtrls[5].syncBpm == 0) {
		EnvLfoCtrls[5].rate = cookVSTGUIVariable(MIN_LFO_RATE, MAX_LFO_RATE, value);
		mainEnvLfo[5].m_dOscFo = EnvLfoCtrls[2].rate;
	}
	else {
		quantDivisor = (int)(value * 10);
		EnvLfoCtrls[5].rate = bpm / quants[quantDivisor];

	}
	sendDoubleAttr(&PMSG_LFO5_RATE_CHAR, "0", EnvLfoCtrls[5].rate);
	mainEnvLfo[5].m_dOscFo = EnvLfoCtrls[5].rate;

	// set lfo.dOscFo to this 
	return 0;
};
int DX10Processor::recalcLfo2Amp(double value) {
	EnvLfoCtrls[5].level = value;
	mainEnvLfo[5].m_dAmplitude = value;
	return 0;
}
int DX10Processor::recalcLfo2Type(double value) { 
	EnvLfoCtrls[5].lfoType = static_cast<int>(div((value * 100), 25).quot);
	return 0;
};

int DX10Processor::recalcLfo2BPM(double value) {
	EnvLfoCtrls[5].syncBpm = (int)value;
	return 0;
};

int DX10Processor::recalcLfo2Delay(double value) {
	EnvLfoCtrls[5].lfoDelay = value;
	return 0;
};


int DX10Processor::recalcLfo3Rate(double value) {
	EnvLfoCtrls[6].rateValue = value;
	EnvLfoCtrls[6].BPM = bpm;
	if (EnvLfoCtrls[6].syncBpm == 0) {
		EnvLfoCtrls[6].rate = cookVSTGUIVariable(MIN_LFO_RATE, MAX_LFO_RATE, value);
		mainEnvLfo[6].m_dOscFo = EnvLfoCtrls[3].rate;
	}
	else {
		quantDivisor = (int)(value * 10);
		EnvLfoCtrls[6].rate = bpm / quants[quantDivisor];
		//EnvLfoCtrls[6].rate = bpm / 60;
		
	}
	sendDoubleAttr(&PMSG_LFO6_RATE_CHAR, "0", EnvLfoCtrls[6].rate);
	mainEnvLfo[6].m_dOscFo = EnvLfoCtrls[6].rate;

	
	return 0;
};
int DX10Processor::recalcLfo3Amp(double value) {
	EnvLfoCtrls[6].level = value;
	mainEnvLfo[6].m_dAmplitude = value;
	return 0;
}
int DX10Processor::recalcLfo3Type(double value) {
	EnvLfoCtrls[6].lfoType = static_cast<int>(div((value * 100), 25).quot);//we need to get this from ctrl notification
	return 0;
};

int DX10Processor::recalcLfo3BPM(double value) {
	EnvLfoCtrls[6].syncBpm = (int)value;
	return 0;
};

int DX10Processor::recalcLfo3Delay(double value) {
	EnvLfoCtrls[6].lfoDelay = value;
	return 0;
};






int DX10Processor::recalcResetWidth(double value) {
	
	return 0;
};





int DX10Processor::recalcToggleGenView(double value) {
	//set paramStartIndex
	//if (generators[1].paramStartIdx == PARTPARAMS) {
	//	generators[1].paramStartIdx = 0;
	//}else {
	//	generators[1].paramStartIdx = PARTPARAMS;
	//}
	//set control view
	//send a message here
	//or intercept in controller

	return 0;
};

int DX10Processor::recalcLfo1Phase(double value) {
	if (EnvLfoCtrls[4].lfoType == 0 ) {
		EnvLfoCtrls[4].phaseStart = (value-0.5)*2;
	}
	else if (EnvLfoCtrls[4].lfoType == 4) {
		EnvLfoCtrls[4].phaseStart = value;
	}
	else {
		EnvLfoCtrls[4].phaseStart = 1-value;
	}
	
	return 0;
}

int DX10Processor::recalcLfo2Phase(double value) {
	if (EnvLfoCtrls[5].lfoType == 0) {
		EnvLfoCtrls[5].phaseStart = (value - 0.5) * 2;
	}
	else if (EnvLfoCtrls[5].lfoType == 4) {
		EnvLfoCtrls[5].phaseStart = value;
	}
	else {
		EnvLfoCtrls[5].phaseStart = 1 - value;
	}
	return 0;
}

int DX10Processor::recalcLfo3Phase(double value) {
	if (EnvLfoCtrls[6].lfoType == 0) {
		EnvLfoCtrls[6].phaseStart = (value - 0.5) * 2;
	}
	else if (EnvLfoCtrls[6].lfoType == 4) {
		EnvLfoCtrls[6].phaseStart = value;
	}
	else {
		EnvLfoCtrls[6].phaseStart = 1 - value;
	}
	return 0;
}


int DX10Processor::recalcStepCtrlTarg(double value) {
	return 0;
}


int DX10Processor::recalcStepCtrl1(double value) {
	sendPidVal(&PMSG_STEP_CTRL_CHAR, STEP_CTRL_1, value);
	return 0;
}

int DX10Processor::recalcStepCtrl2(double value) {
    sendPidVal(&PMSG_STEP_CTRL_CHAR, STEP_CTRL_2, value);
	return 0;
}
int DX10Processor::recalcStepCtrl3(double value) {
    sendPidVal(&PMSG_STEP_CTRL_CHAR, STEP_CTRL_3, value);
	return 0;
}
int DX10Processor::recalcStepCtrl4(double value) {
    sendPidVal(&PMSG_STEP_CTRL_CHAR, STEP_CTRL_4, value);
	return 0;
}
int DX10Processor::recalcStepCtrl5(double value) {
    sendPidVal(&PMSG_STEP_CTRL_CHAR, STEP_CTRL_5, value);
	return 0;
}
int DX10Processor::recalcStepCtrl6(double value) {
    sendPidVal(&PMSG_STEP_CTRL_CHAR, STEP_CTRL_6, value);
	return 0;
}
int DX10Processor::recalcStepCtrl7(double value) {
    sendPidVal(&PMSG_STEP_CTRL_CHAR, STEP_CTRL_7, value);
	return 0;
}
int DX10Processor::recalcStepCtrl8(double value) {
    sendPidVal(&PMSG_STEP_CTRL_CHAR, STEP_CTRL_8, value);
	return 0;
}
int DX10Processor::recalcStepCtrl9(double value) {
    sendPidVal(&PMSG_STEP_CTRL_CHAR, STEP_CTRL_9, value);
	return 0;
}
int DX10Processor::recalcStepCtrl10(double value) {
    sendPidVal(&PMSG_STEP_CTRL_CHAR, STEP_CTRL_10, value);
	return 0;
}
int DX10Processor::recalcStepCtrl11(double value) {
    sendPidVal(&PMSG_STEP_CTRL_CHAR, STEP_CTRL_11, value);
	return 0;
}
int DX10Processor::recalcStepCtrl12(double value) {
    sendPidVal(&PMSG_STEP_CTRL_CHAR, STEP_CTRL_12, value);
	return 0;
}
int DX10Processor::recalcPanLR(double value) {
	

	return 0;
}

int DX10Processor::recalcPanLREnv(double value) {
	panEnv = value;
	//int pIdx = TUNE_ENV + paramStartIdx;
	//controls[TUNE_ENV] = 2.0 * (*partParams[pIdx]) - 1;
	//controls[TUNE_ENV] = (*partParams[pIdx]);
	return 0;
}

int DX10Processor::recalcPanLREnvSrc(double value) {
	
	panEnvSrc = div(static_cast<int>(value * 100), 14).quot;

	//mainEnvLfo[panEnvSrc].cenv * xxx
	//int pIdx = TUNE_ENV_SRC + paramStartIdx;
	//controls[TUNE_ENV_SRC] = ctrlValues[pIdx];
	return 0;
}

//THIS IS WHERE WE GET PARAM ID ON PARAM CONTROL CHANGE
bool DX10Processor::processParameterChanges(IParameterChanges* changes){

	//BaseProcessor::processParameterChanges(changes);
	//BRANCHINE
	if (changes){
		int32 count = changes->getParameterCount();
		if (count > 0){
			for (int32 i = 0; i < count; i++){
				IParamValueQueue* queue = changes->getParameterData(i);
				if (!queue) {
					continue;
				}
					
				ParamID paramId = queue->getParameterId();
				int32 pointCount = queue->getPointCount();
				int32 sampleOffset;
				ParamValue value;
				queue->getPoint(pointCount - 1, sampleOffset, value);

				if (paramId < NPARAMS) {
					// this is only going to process PER GENERATOR params, not global ones
					//lets go like process gloabal params
					//then process generator params
					floatParams[paramId] = value;
					int genIdx = div(paramId, PARTPARAMS).quot;//division better than for loop?
					int paramIdx = paramId - (PARTPARAMS*genIdx);
					generators[genIdx].recalc2(paramIdx);

					
				}
				else if (paramId < GPARAMS) {//not leass than partparams but less than global params ie idx between 81 and 1000
					floatParams[paramId] = value;
					procValues[paramId] = value;
					(this->*recalculators[paramId])(value);
				}

				else if (paramId == BaseController::kBypassParam) {
					setBypass(value >= 0.5, sampleOffset);
				}else if (paramId == BaseController::kPresetParam) {
					setCurrentProgramNormalized(value);

				}else if (paramId == BaseController::kLinkGen1Param) {//REMOVE
					if (value > 0.0) {
						
					}
					else {
						
					} 

				}else if (paramId == BaseController::kPresetParam) { // program change

					currentProgram = std::min<int32>(kNumPrograms - 1, (int32)(value * kNumPrograms));
					const float* newParams = programParams[currentProgram];
					if (newParams) {
						for (int32 i = 0; i < GPARAMS; i++) {
							floatParams[i] = newParams[i];
						}

					}

				}
				else if (paramId == BaseController::kModWheelParam) { // mod wheel
					value *= 127.;
					generators[0].modwhl = 0.00000005f * (float)(value * value);
					//generators[1].modwhl = 0.00000005f * (float)(value * value);
				}
				else if (paramId == BaseController::kPitchBendParam) { // pitch bend
					if (value <= 1) {
						generators[0].pbend = (value - 0.5) * 0x2000;
						//generators[1].pbend = (value - 0.5) * 0x2000;
					}
					else {
						generators[0].pbend = value;
						//generators[1].pbend = value;
					}


					if (generators[0].pbend > 0.0f) {
						generators[0].pbend = 1.0f + 0.000014951f * generators[0].pbend;
					}
					else {
						generators[0].pbend = 1.0f + 0.000013318f * generators[0].pbend;
					}
					/*
					if (generators[1].pbend > 0.0f) {
						generators[1].pbend = 1.0f + 0.000014951f * generators[1].pbend;
					}
					else {
						generators[1].pbend = 1.0f + 0.000013318f * generators[1].pbend;
					}
					*/
				}

				
					
			}
			return true;
		}
	}
	return false;
}


tresult DX10Processor::setState(IBStream* state) {
	BaseProcessor::setState(state);
	//////////////////////////////////////////////
	for (int i = 0; i < PARTPARAMS; i++) {
		generators[0].recalc2(i);
	}
	//for vertSwitchPids[_GEN_STEP_PARAM_COUNT_] send message to changeVertSwitch?
	//seems there should be a better way is there a similar set program method in ctrolers?

	for (int i = ENV_1_ATTACK; i < STEP_CTRL_TARG + 1; i++) {//DO WE NEED THIS// initializes levels
		(this->*recalculators[i])(floatParams[i]);
		//sendPidVal(&PMSG_STEP_SWITCH2_CHAR, i, floatParams[i]);
	}
	//message->getAttributes()->getInt("0", pid);
	//message->getAttributes()->getFloat("1", value);

	for (int i = 0; i < _GEN_STEP_PARAM_COUNT_; i++) {
		loadPID = vertSwitchPids[i];
		loadVal = floatParams[loadPID];
		//sendPidVal(&PMSG_STEP_SWITCH2_CHAR, loadPID, loadVal);
		sendPidVal(&PMSG_STEP_SWITCH2_CHAR, loadPID, loadVal);
	}
	for (int i = 0; i < TOGGLE_COUNT; i++) {
		loadPID = toggleBtnPids[i];
		loadVal = floatParams[loadPID];
		//sendPidVal(&PMSG_STEP_SWITCH2_CHAR, loadPID, loadVal);
		sendPidVal(&PMSG_STEP_SWITCH2_CHAR, loadPID, loadVal);
	}

	for (int i = 0; i < GENRAMPEDPARAMSLENGTH; i++) {
		generators[0].rampedParams[i].initLevel(floatParams[generators[0].rampedList[i]]);
	}
	////////////////////////////////////////////

	
	BaseProcessor::recalculate();

	return kResultTrue;
}

}}} // namespaces

