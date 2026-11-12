
#ifndef __DXGENERATOR_H_INCLUDED__
#define __DXGENERATOR_H_INCLUDED__

#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "Map.h"
#include "base/source/timer.h"
#include "RVoice.h"
#include "REvents.h"
#include "REventable.h"
#include "immintrin.h"

//#include "mdaBaseprocessor.h"
//#include "mdaDX10Processor.h"
#include <deque>
#include "ParamRampObj.h"
#include "WTTables.h"
/*
#include "SynthParamLimits.h"
//#include "SynthFunctions.h"
#include "MoogLadderFilter.h"
#include "logscale.h"
*/


namespace Steinberg {
namespace Vst {
namespace rs {

typedef unsigned int UINT;


inline int32_t RSMAX(int32_t a, int32_t b) { return((a) > (b) ? a : b); }
inline int32_t RSMIN(int32_t a, int32_t b) { return((a) < (b) ? a : b); }

class MsgDef {
public:
	MsgDef(const char* msgIn, const char* attrIn, double valueIn) {
		msg = msgIn;
		attr = attrIn;
		value = valueIn;
	};
	~MsgDef();

	const char* msg;
	const char* attr;
	double value;
};

class DX10Processor;

// B-06: MIDI velocity (1..127) -> amp-envelope gain, linear. The old formula
// 0.01 * (100 / (127 / v)) used integer division: only 20 distinct gains, every velocity from 64 to
// 127 gave 1.0, velocity 1 gave 0, and velocity 0 would divide by zero. Change the curve here.
inline float velocityGain(int32 v) {
	if (v < 1) v = 1;
	if (v > 127) v = 127;
	return static_cast<float>(v) / 127.0f;
}

// B-10: fixed-capacity, allocation-free render list (was std::deque<int32>, which allocated on the
// audio thread as it grew). Holds at most one entry per pitch; order is preserved on erase.
struct VoiceList {
	static const int kCapacity = 128;
	int32 items[kCapacity];
	int count = 0;
	int size() const { return count; }
	int32& operator[](int i) { return items[i]; }
	const int32& operator[](int i) const { return items[i]; }
	bool push_back(int32 x) {
		if (count >= kCapacity) return false;
		items[count++] = x;
		return true;
	}
	void eraseAt(int i) {
		for (int k = i; k < count - 1; k++) items[k] = items[k + 1];
		count--;
	}
};

// this defines a logarithmig scaling for the filter Fc control

//class DXGenerator : public REventable, public RDataConsumer, public FObject, public ITimerCallback, public Constants  {
class DXGenerator : public REventable, public RDataConsumer, public FObject {
private:
	
public:
	DXGenerator() ;

	void init();

	DXGenerator::~DXGenerator() {
		delete[] RVoicesMap;  // B-24: was never freed (128 voices per plugin instance)
		RVoicesMap = nullptr;
		delete filterLogScale2;
		delete m_pFilterLogParam;
		delete[] ddMap;
		if (doRecalc) {
			_aligned_free(doRecalc);
			doRecalc = nullptr;
		}

		// Set pointers to nullptr to avoid dangling references
		filterLogScale2 = nullptr;
		m_pFilterLogParam = nullptr;
		ddMap = nullptr;
		//doRecalc = nullptr;
	}
	//Constants constList;

	//std::deque<MsgDef> msgStack;

	int linkTune = 0;
	int linkFine = 0;
	//BaseProcessor* baseProcessor;
	//void sendParams() override { onUpdate(); }
	//std::function<void()> onUpdate;

	int paramStartIdx;

	DX10Processor* parentProcessor;
	
	CLFO syncLFO;
	//array of DXGenerator::methods with signature int(RVoice&) ie. returns int 
	//
	//{rettype}({ClassNAme}::*{nameOFTheACtualArray}[arraySize](inputType(S))
	//
	

	float combineMultiply(float valA, float valB);


	float combineAdd(float valA, float valB);



	int mainInt = 0;
	int mainOct = 0;
	int mainNote = 0;
	int mainTune = 0;
	int mainFine = 0;
	
	int modInt = 0;
	int modOct = 0;
	int modTune = 0;
	int modFine = 0;
	
	//float combineAdd(float valA, float valB);

	//float combineMultiply(float valA, float valB);

	float (DXGenerator::*combineValues[2])(float, float);
	float filterEnv = 0.0;
	int filterEnvSrc = 0;
	int filterSrc = 0.0f;
	int filterAddMult = 1;
	int filterAddMultCtrl = 0;
	int newInt = 0;
	int newOct = 0;
	int newIntEnv = 0;
	int newModOctEnv = 0;
	int newModIntEnv = 0;
	int tuneCEnv = 0;
	int fineCEnv = 0;
	int modTuneCEnv = 0;
	int ogModTuneCEnv = 0;
	int modTuneEnv = 0;
	int ogModTuneEnv = 0;
	int modFineCEnv = 0;
	int newOctEnv = 0;
	int newModOct = 0;
	int newModInt = 0;
	int squareEnvSrc = 0;
	int modTriEnvSrc = 0;
	int modSquareEnvSrc = 0;
	int triEnvSrc = 0;
	int newIntCtrl = 0;
	int newOctCtrl = 0;
	int newModOctCtrl = 0;
	int newModIntCtrl = 0;

	int panEnvSrc;
	int modGainEnvSrc;

	float o1 = 0.0;
	float o2 = 0.0;
	float gainer = 0.0;
	float clipper = 1.0;
	float tuneC;
	float tuneM;
	//widthPole is set on width recalc to represent width level sign;
	float widthPole = 1.0;
	float widthEdge = -0.1;
	float modWidthPole = 1.0;
	float modWidthEdge = -0.1;
	Env* EnvLfoCtrls[7];

	int modGainMod1Toggle = 0;
	int modGainMod2Toggle = 0;
	float modGainMod1Level = 0.0;
	float modGainMod2Level = 0.0;

	float* mainCEnvPtr;

	void recalc2(int paramId);

	int(DXGenerator::*modChoice[4])(RVoice*);

	int doPhaseMod(RVoice* voiceObj);

	int doFreqMod(RVoice* voiceObj);

	int doDetuneMod(RVoice* voiceObj);

	int doNoMod(RVoice* voiceObj);
	
	//int(DXGenerator::*doRecalc[PARTPARAMS])();

	typedef int (DXGenerator::* recalcFunc_t)();

	recalcFunc_t* doRecalc;

	int(DXGenerator::*processEnv[ENV_COUNT])();

	float widthInc = 0.0;

	int ctrlValues[NPARAMS] = {0};//maybee this should be 

	int toggleModGainMod1 = 0;
	int toggleModGainMod2 = 0;
	int toggleModGain2Mod1 = 0;
	int toggleModGain2Mod2 = 0;

	int recalcMainOsc();//0

	int recalcWidth();
	int recalcWidthEnv();
	int recalcWidthEnvSrc();

	int recalcTune();
	int recalcTuneEnv();
	int recalcTuneEnvSrc();

	int recalcFine();
	int recalcFineEnv();
	int recalcFineEnvSrc();


	int recalcOct();
	int recalcOctEnv();
	int recalcOctEnvSrc();

	int recalcInt();
	int recalcIntEnv();
	int recalcIntEnvSrc();

	int recalcTuneMod1Toggle();
	int recalcTuneMod2Toggle();
	int recalcTuneMod1();
	int recalcTuneMod2();//12

	int recalcModOsc();
	int recalcModType();

	int recalcModGain();
	int recalcModGainEnv();
	int recalcModGainEnvSrc();

	int recalcModWidth();
	int recalcModWidthEnv();
	int recalcModWidthEnvSrc();


	int recalcModTune();
	int recalcModTuneEnv();
	int recalcModTuneEnvSrc();


	int recalcModFine();
	int recalcModFineEnv();
	int recalcModFineEnvSrc();

	int recalcModOct();
	int recalcModOctEnv();
	int recalcModOctEnvSrc();

	int recalcModInt();
	int recalcModIntEnv();
	int recalcModIntEnvSrc();

	int recalcModBeat();
	int recalcModBeatEnv();
	int recalcModBeatEnvSrc();



	int recalcModMod1Toggle();
	int recalcModMod2Toggle();
	int recalcModMod1();
	int recalcModMod2();//21

	int recalcGain();
	int recalcGainEnv();
	int recalcGainEnvSrc();

	int recalcFilter();
	int recalcFilterEnv();//26
	int recalcFilterEnvSrc();

	int recalcFilterQ();

	int recalcPhaseModToggle();//unused32
	int recalcfreqModToggle();//used33
	int recalcBeatFreq();//34
	int toggleBeatFreq();//35unused

	int recalcLFORate();
	int recalcLfoOsc();
	int recalcLfoBpm();
	int recalcLfoAmp();//aux4/39

	int recalcPulseWidth();

	int recalcEnvAtt();//40
	int recalcEnvDec();//41
	int recalcEnvRel();//42


	int recalcAttack();
	int recalcDecay();
	int recalcReleaseToggle();
	int recalcRelease();

	int recalcModOscGain();
	int recalcSyncGenModTune();
	int recalcSyncGenModFine();

	int recalcPanLR();//49
	int recalcPanLREnv();//50
	int recalcPanLREnvSrc();//50

	int doNothing();//42

	float voiceEnvAtt;
	float voiceEnvDec;
	float voiceEnvRel;

	float paramInc = 0.0f;
	float panSmoothCoef = 0.0075f;  // B-55: one-pole pan glide coefficient (~3 ms), set in createVoice
	float paramThresh = 0.0f;

	ParamRampObj rampedParams[GENRAMPEDPARAMSLENGTH];
	int nSize;
	/*
	void updateLfo1Amp(float level);
	void updateLfo2Amp(float level);
	void updateLfo3Amp(float level);


	void updateLfo1FO( float rate );
	void updateLfo2FO(float rate );
	void updateLfo3FO(float rate );


	void updateLfo1Phase(float phase);
	void updateLfo2Phase(float phase);
	void updateLfo3Phase(float phase);
	*/
	int ctrlValuesINT_;
	int ctrlValuesOCT_;
	int ctrlValuesMOD_INT_;
	int ctrlValuesMOD_TYPE_;
	int ctrlValuesMOD_OCT_;
	float controlsINT_ENV;
	float controlsOCT_ENV;
	float controlsTUNE_;
	float controlsTUNE_ENV;
	float controlsFINE_;
	float controlsFINE_ENV;
	float controlsWIDTH_ENV;
	float controlsMOD_INT_ENV;
	float controlsMOD_OCT_ENV;
	float controlsMOD_TUNE_;
	float controlsMOD_TUNE_ENV;
	float controlsMOD_FINE_;
	float controlsMOD_FINE_ENV;
	float controlsMOD_WIDTH_ENV;
	float controlsGAIN_ENV;
	float controlsFILTER_ENV;
	float controlsPAN_LR;
	float controlsPAN_LR_ENV;
	float controlsMOD_GAIN_ENV;
	float controlsMOD_BEAT_;
	float controlsMOD_BEAT_ENV;

	float rampedParamsWIDTH_RAMPINDEX_level;
	float rampedParamsMOD_WIDTH_RAMPINDEX_level;
	float rampedParamsFILTER_RAMPINDEX_level;
	float rampedParamsGAIN_RAMPINDEX_level;
	float rampedParamsMOD_OSC_GAIN_RAMPINDEX_level;
	float rampedParamsPAN_LR_RAMPINDEX_level;
	float rampedParamsPAN_LR_ENV_RAMPINDEX_level;
	float rampedParamsMOD_GAIN_RAMPINDEX_level;
	//unused?
	int rampedList[GENRAMPEDPARAMSLENGTH] = {
		WIDTH_
		, GAIN_
		, FILTER_
		, MOD_GAIN_
		, MOD_WIDTH_
		, MOD_OSC_GAIN
		, PAN_LR
		, PAN_LR_ENV
		//, LFO_1_AMP THESE GO IN PROCESSOR
		//, LFO_2_AMP
		//, LFO_3_AMP
		};

	bool cutNote = true;

	float gainLevel;

	float ENV_MIN_CUTOFF = 0.0025f;

	const double* dBT = dBLEPTable;

	CWTTables WTTables;

	inline float cookVSTGUIVariable(float fMin, float fMax, float fVar){
		return (fMax - fMin)*fVar + fMin;
	}

	void processVoices(int noteSize);

	void ParamClamp(float* n);
	void ParamClamp(double* n);
	
	float panDiff;

	//double  ddMap[1000];
	double* ddMap;
	
	LogScale<ParamValue>* filterLogScale2;
	Parameter* m_pFilterLogParam;

	/*inline float get_m_dFcControl( Steinberg::Vst::Parameter* logParam, float ctrlLevel, float filterEnv) {
		float normalized = ParamClamp2( ctrlLevel+filterEnv, 0.01, 1.0 );
		return logParam->toPlain(normalized);
	}*/


	float filterCtrlFreq;

	double carPitchMultiplier = 0.0;
	double modPitchMultiplier = 0.0;
	double pitchBendMod = 0.0;
	double semiToneMod = 0.0;
	double centMod = 0.0;

	double m_dFcControl;
	double m_dQControl;
	UINT m_uFilterKeyTrack;
	double m_dFilterKeyTrackIntensity;
	double m_dFilterModRange;

	
	int tuneOctInt;
	int tuneIntInt;
	
	int modOctInt;
	//float modTune;
	//float modFine;
	float modIntInt;
	float beatFreqInt;
	float ratioBounds;
	float decayReleaseToggle;
	float filterRez;
	float filterEnvAmp;
	div_t divresult;

	float controls[GPARAMS];

	


	int modTypeSelectorInt = 0;
	
	double lastFreq = 0.0;
	int lastFreqSign = -1;
	double lastFreqDif = 0.0;
	double portoInc = 0.0;

	double lastModFreq = 0.0;
	int lastModFreqSign = -1;
	double lastModFreqDif = 0.0;
	double portoModInc = 0.0;

	float portoRate = 0.5f;

	RVoice monoVoice;

	float baseVal;

	int newNote;

	float modGainLevel;
	float modGainTarg;
	float modGainInc;

	float ratioPImX;
	CMoogLadderFilter m_Filter1;
	double bpm = 120.0;
	

	void createVoice();

	float adjustedRatio = 0.0f;

	void processNoteOn(RVoice& voiceObj, int32* note, int32* vel, bool retrig = false);  // retrig: B-56

	void processNoteReOn(RVoice& voiceObj, int32* note, int32* vel);

	void saveTargValues(RVoice& voiceObj, bool killIt);

	bool interpolateFloat(float& origVal, float& targVal, float& val);
	float interpolateFS;

	void resetFinished(RVoice& voiceObj);

	void interpolateVoice(RVoice& voiceObj);

	void retriggerVoice(RVoice& voiceObj, bool killIt);

	void singleVoiceProcessing(RVoice& voiceObj);
	
	void deadVoiceProcessing(RVoice& voiceObj);

	void calculateSingleOut(RVoice & voiceObj);

	
	
	
	float raspyLevel = 1.0f;
	float smoothLevel = 1.0f;
	float modulatedLevel = 1.0f;

	int bb = 0;

	
	double osc2Out;
	

	//float sendCoarseEnv1[3] { 0f, 0f, 0f };
	float sampleRate;
	double sampleRateDouble;
	float halfSecSamples;

	Map::hash<std::string, int>* paramKeys;

	int genIdx;

	int quietestVoice;
	int processVoiceCounter = 0;
	RVoice* processVoice;
	//REvents Voices;
	double targOscFo;
	//RVoice RVoicesMap[PARTVOICES];
	RVoice * RVoicesMap = nullptr;  // B-24
	float foDifference = 0.0f;
	bool seekAndReplace;

	double lastOscFo = 0.0;
	double mergedOscFo = 0.0;
	//float portoInc = 0.0;
	int32 quietest = 0;
	int32 notesSize = 0;
	//TRY MAKING VOICES MAP OUTPUT BUFFER TO SUM AVERAGE output?
	float RVoicesOutput[PARTVOICES];
	int32 POLYMODE = 0;
	int32 POLYCOUNT = 32;//try 8 for oct
	int32 lastNoteIdx = 0;
	bool pushNote;
	int32 notes3[10];
	int32 voiceCounter = 0;
	VoiceList notes;  // pitches being rendered (B-17/B-10)
	std::deque<RVoice> notes2;

	//ADD THESE TO VOICE?
	//or make these as event data in rsogview
	float tune;
	float rati;
	float ratix;
	float ratf;
	float ratio; //modulator ratio
	float catt;
	float attCP;
	float cdec;
	float targCdec;
	float setCdec;
	float crel;        //carrier envelope
	float depth;
	float sustDepth;
	float sust;

	float lfo0;
	float lfo1;
	float dlfo;
	float modwhl;
	float MW;
	float pbend;
	float velsens;
	float volume;
	float vibrato; //LFO and CC
	float rich;
	float modmix;
	
	float E1catt;
	float E1cdec;
	float E1cdecay;
	float E1crel;
	float E1e;

	
	float EOffCatt;
	float EOffCdec;
	float EOffCdecay;
	float EOffCrel;
	float EOffE;


	int v;
	int vl;
	float l;
	float lO;
	float o;
	float x;
	float y;
	float z;
	float e; 
	float mw;
	float w;
	float m;
	int32 k;

	float lastO;

	float xLev;
	float xLevTarg = 0.0;
	float xLevInc = 0.0;
	//CParamSmooth xLevSmooth;
	float swLev;
	float swLevTarg = 0.0;
	float swLevInc = 0.0;
	//CParamSmooth swLevSmooth;
	float sinLev;
	float sinLevTarg = 0.0;
	float sinLevInc = 0.0;
	//CParamSmooth sinLevSmooth;
	float modOne;
	float modOneTarg = 0.0;
	float modOneInc = 0.0;
	//CParamSmooth modOneSmooth;


	float lfoRateCtrl;

	int32 removals[121];

	int32 voicePointers[64];

	int32 sampleFrame;
	int32 event = 0;
	int32 frame = 0;
	int32 frames;
	float* gOut1;
	float* gOut2;
	int32 sampleFrames;

	float interpDist;
	int32 sustain;
	int32 activevoices;
	int32 K;
	float cdecay;
	float ifs;
	int32 dv;
	int32 dx;
	int activeVoices;
	float silenceCutoff;
	int32 sustainNote;

	bool rampingDown = false;

	ParamValue* params;

	ParamValue* partParams[NPARAMS];

	ParamValue* env1Params[NPARAMS];

	int syncModTuneCtrl[2];

	int syncModFineCtrl[2];

	void noteOn( int32* note, int32* velocity, int32* noteID );
	void noteOff(int32* note, int32* noteID);
	void releaseVoice(RVoice& voice);
	void recalculate(const float& sampleFloat );
	void recalculateRel(const float& sampleFloat, RVoice& voiceObj);
	void detectSilence(int32 * activeVoices);
	void DXGenerator::signalProcessing();
	virtual void mainProcessing() override;
	//virtual void voiceProcessing(RVoice& obj) override;
	float getRelVal1D(float rangeAStart, float rangeAStop, float rangeBStart, float rangeBStop, float value);
	void setActive();
	float sinLevDxSign;

	/*
	__m256 newInt_magnt = _mm256_setzero_ps();

	__m256 newInt_env_cenv = _mm256_setzero_ps();

	__m256 newInt_controls_intrv_env = _mm256_setzero_ps();

	__m256 newInt_ctrlValues_intrv = _mm256_setzero_ps();

	__m256 newModInt_magnt = _mm256_setzero_ps();

	__m256 newModInt_env_cenv = _mm256_setzero_ps();

	__m256 newModInt_controls_intrv_env = _mm256_setzero_ps();

	__m256 newModInt_ctrlValues_intrv = _mm256_setzero_ps();


	

	__m256 newOct_magnt = _mm256_setzero_ps();
	
	__m256 newOct_ctrl_offset = _mm256_setzero_ps();

	__m256 newOct_env_cenv = _mm256_setzero_ps();

	__m256 newOct_controls_intrv_env = _mm256_setzero_ps();

	__m256 newOct_ctrlValues_intrv = _mm256_setzero_ps();

	__m256 newOctMod_magnt = _mm256_setzero_ps();

	__m256 newOctMod_ctrl_offset = _mm256_setzero_ps();

	__m256 newOctMod_env_cenv = _mm256_setzero_ps();

	__m256 newOctMod_controls_intrv_env = _mm256_setzero_ps();

	__m256 newOctMod_ctrlValues_intrv = _mm256_setzero_ps();


	__m256 newNote_magnt = _mm256_setzero_ps();

	__m256 newNote_set = _mm256_setzero_ps();

	__m256 newNote_freq = _mm256_setzero_ps();

	__m256 tuneC_magnt = _mm256_setzero_ps();

	__m256 tuneC_env_fine_cenv = _mm256_setzero_ps();

	__m256 tuneC_env_fine_env = _mm256_setzero_ps();

	__m256 tuneC_env_fine = _mm256_setzero_ps();

	__m256 tuneC_env_tune_cenv = _mm256_setzero_ps();

	__m256 tuneC_env_tune_env = _mm256_setzero_ps();

	__m256 tuneC_env_tune = _mm256_setzero_ps();

	__m256 newNote_oct_magnt = _mm256_setzero_ps();

	__m256 newNote_min_bound = _mm256_setzero_ps();

	__m256 tri_cut_env_src = _mm256_setzero_ps();

	__m256 tri_cut_env = _mm256_setzero_ps();

	__m256 tri_cut_pole = _mm256_setzero_ps();

	__m256 tri_cut_ctrl = _mm256_setzero_ps();

	__m256 tri_cut_env_min = _mm256_setzero_ps();

	__m256 tri_cut_env_max = _mm256_setzero_ps();


	__m256 newNoteMod_magnt = _mm256_setzero_ps();

	__m256 newNoteMod_set = _mm256_setzero_ps();

	__m256 newNoteMod_freq = _mm256_setzero_ps();

	__m256 tuneCMod_magnt = _mm256_setzero_ps();

	__m256 tuneCMod_env_fine_cenv = _mm256_setzero_ps();

	__m256 tuneCMod_env_fine_env = _mm256_setzero_ps();

	__m256 tuneCMod_env_fine = _mm256_setzero_ps();

	__m256 tuneCMod_env_tune_cenv = _mm256_setzero_ps();

	__m256 tuneCMod_env_tune_env = _mm256_setzero_ps();

	__m256 tuneCMod_env_tune = _mm256_setzero_ps();

	__m256 newNoteMod_oct_magnt = _mm256_setzero_ps();

	__m256 newNoteMod_min_bound = _mm256_setzero_ps();

	__m256 newNoteMod_upper_bound = _mm256_setzero_ps();

	__m256 newNoteMod_lower_bound = _mm256_setzero_ps();

	__m256 triMod_cut_env_src = _mm256_setzero_ps();

	__m256 triMod_cut_env = _mm256_setzero_ps();

	__m256 triMod_cut_pole = _mm256_setzero_ps();

	__m256 triMod_cut_ctrl = _mm256_setzero_ps();


	float storeNewNote[8];
	float storeNewNoteMod[8];

	float storeFO[8][8];
	float storeModFO[8][8];
	float storeTriCut[8][8];
	float storeModTriCut[8][8];
	*/

	int32 notesTemp[32];
	


};



}
}
}

#endif

