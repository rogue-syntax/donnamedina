#pragma once
#include "pluginconstants.h"
#include "synthfunctions.h"
#include "ModulationMatrix.h"
#include "SineWave.h"

#define OSC_FO_MOD_RANGE 2			//2 semitone default
#define OSC_HARD_SYNC_RATIO_RANGE 4	//4
#define OSC_PITCHBEND_MOD_RANGE 12	//12 semitone default
#define OSC_FO_MIN 20				//20Hz
#define OSC_FO_MAX 20480			//20.480kHz = 10 octaves up from 20Hz
#define OSC_FO_DEFAULT 440.0		//A5
#define OSC_PULSEWIDTH_MIN 2		//2%
#define OSC_PULSEWIDTH_MAX 98		//98%
#define OSC_PULSEWIDTH_DEFAULT 50	//50%

class CLFOscillator
{
public:
	CLFOscillator(void);
	virtual ~CLFOscillator(void);	

	bool isModulator = false;
	//CLFOscillator* twinOsc;
	// --- ATTRIBUTES
	// --- PUBLIC: these variables may be get/set 
	//             you may make get/set functions for them 
	//             if you like, but will add function call layer
	//
	// --- Modulation Matrix --------------------
	//
	// --- the shared modulation matrix
	CModulationMatrix* m_pModulationMatrix;	// the matrix//REMOVE
	
	// --- sources that we read from
	//	   indexes in m_pModulationMatrix->Sources[]
	UINT m_uModSourceFo; 
	UINT m_uModSourcePulseWidth; 
	UINT m_uModSourceAmp;

	// --- destinations that we write to
	//	   indexes in m_pModulationMatrix->Destinations[]
	UINT m_uModDestOutput1;
	UINT m_uModDestOutput2;
	// ------------------------------------------
	
	// --- Global Parameters --------------------
	//
	globalOscillatorParams* m_pGlobalOscParams;//REMOVE
	// ------------------------------------------
	int note;
	// --- oscillator run flag
	bool m_bNoteOn;

	// --- user controls or MIDI 
	float m_dOscFo;	
	float m_dOscFoPrev;	// oscillator frequency from MIDI note number
	float m_dFoRatio;	    // FM Synth Modulator OR Hard Sync ratio 
	float m_dAmplitude;	// 0->1 from GUI

	float m_dDelayTime = 0.5f; 
	float m_dDelayCounter = 0.0f;
	float m_dDelayForce = 0.25f;
	// --- modulo counter and inc for timebase
	float m_dModulo;		// modulo counter 0->1
	float m_dDelayInc;
	//sine pattern : .0 -> .25 :up, .25 -> .75 down, .75 -> 1.0: up
	//tri pattern : .0 -> dCalcModulo * triCuttof : up, dCalcModulo * triCuttof -> 0.1 :down (COULD BE BCKWARDS IN DOWNRTI VS UPTRI)
	//square pattern : SAME:... .0 -> dCalcModulo * triCuttof : up, dCalcModulo * triCuttof -> 0.1 :down (COULD BE BCKWARDS IN DOWNSQ VS UPSQ)
	float m_dInc;			// phase inc = fo/fs
	int tempModulo;
	float moduloStart;
	float lastModulo;
	float lastDInc;
	div_t moduloDiv;
	// --- more pitch mods
	int m_nOctave;			// octave tweak
	int m_nSemitones;		// semitones tweak
	int m_nCents;			// cents tweak
	
	// ---  pulse width in % (sqr only) from GUI
	float m_dPulseWidthControl;	
	
	// --- for PITCHED Oscillators
	enum {SINE,SAW1,TRI, SQUARE,SAW2,SAW3,NOISE,PNOISE,RSINE};
	UINT m_uWaveform = 0;	// to store type
	
	// --- for LFOs
	enum {sine,usaw,dsaw,tri,square,expo,rsh,qrsh};

	// --- for LFOs - MODE
	enum {sync,shot,free};
	UINT m_uLFOMode;	// to store MODE
	
	// --- for hard sync or other dual-oscillator ideas
	//CLFOscillator* m_pBuddyOscillator;

	// --- flag indicating we are a master oscillator
	bool m_bMasterOsc;
	
	// --- MIDI note that is being played
	UINT m_uMIDINoteNumber;
	
	float startingSampleRate = 44100.0;

	// --- PROTECTED: generally these are either basic calc variables
	//                and modulation stuff
	// --- calculation variables
	float* m_dSampleRatePtr;
	double* m_dSampleRate;
	float m_dSampleRateF;
	// fs
	float m_dFo;			// current (actual) frequency of oscillator	
	float m_dPulseWidth = 50.0;	// pulse width in % for calculation
	bool m_bSquareEdgeRising; // hysteresis for square edge
	
	// --- for noise and random sample/hold
	UINT   m_uPNRegister;	// for PN Noise sequence
	int    m_nRSHCounter;	// random sample/hold counter
	float m_dRSHValue;		// currnet rsh output
	
	// --- for DPW
	float m_dDPWSquareModulator;	// square toggle
	float m_dDPW_z1; // memory register for differentiator
	
	// --- mondulation inputs
	float m_dFoMod;			/* modulation input -1 to +1 */
	float m_dPitchBendMod;	    /* modulation input -1 to +1 */
	float m_dFoModLin;			/* FM modulation input -1 to +1 (not actually used in Yamaha FM!) */
	float m_dPhaseMod;			/* Phase modulation input -1 to +1 (used for DX synth) */
	float m_dPWMod;			/* modulation input for PWM -1 to +1 */
	float m_dAmpMod;			/* output amplitude modulation for AM 0 to +1 (not dB)*/
		
	float triCutoff = 0.5;

	// --- FUNCTIONS: all public
	//
	// --- modulo functions for master/slave operation
	// --- increment the modulo counters
	inline void incModulo(){

		m_dModulo += m_dInc;

	
	}

	//int(CLFOscillator::*onWrap[1])();
	//int onWrapDoNothing();
	//int onWrapExecute();
	int testIt = 0;
	int testsync = 0;
	int testOsc = 0;
	int testMethod = 0;

	int wrappingBool = 0;
	int modTypeSelector = 0;
	// --- check and wrap the modulo
	//     returns true if modulo wrapped

	inline bool checkWrapModulo() {

		if (m_dInc > 0 && m_dModulo >= 1.0)
		{
			m_dModulo -= 1.0;
			return true;
		}
		// --- for negative frequencies
		else if (m_dInc < 0 && m_dModulo <= 0.0)
		{
			m_dModulo += 1.0;
			return true;
		}
		return false;
	}

	inline int checkWrapModulo2(){	


		wrappingBool = (int)(m_dModulo);

		m_dModulo = m_dModulo - wrappingBool;

		return wrappingBool;

	}

	inline int checkWrapModuloTri() {

		wrappingBool = (int)m_dModulo;

		m_dModulo = m_dModulo - wrappingBool;

		if (m_dModulo < triCutoff) {
			wrappingBool = 0;
		}
		else {
			wrappingBool = 1;
		}
		return wrappingBool;

	}
	

	inline int oldWrapModulo() {
		// --- for positive frequencies
		/*
		if(m_dInc > 0 && m_dModulo >= 1.0)
		{
			if (isModulator != true) {
				if (modTypeSelector == 1) {
					(*twinOsc).m_dModulo = 0.0;
				}
			}
			m_dModulo -= 1.0;
			return true;
		}
		// --- for negative frequencies
		if(m_dInc < 0 && m_dModulo <= 0.0)
		{

			if (isModulator != true) {
				if (modTypeSelector == 1) {
					(*twinOsc).m_dModulo = 0.0;
				}
			}
			m_dModulo += 1.0;
			return true;
		}

		*/
	}

	// --- reset the modulo (required for master->slave operations)
	inline void resetModulo(float d = 0.0){m_dModulo = d;}

	// --- modulation functions - NOT needed/used if you implement the Modulation Matrix!
	//
	// --- output amplitude modulation (AM, not tremolo (dB); 0->1, NOT dB
	inline void setAmplitudeMod(float dAmp){m_dAmpMod = dAmp;}

	// --- modulation, exponential
	inline void setFoModExp(float dMod){m_dFoMod = dMod;}
	inline void setPitchBendMod(float dMod){m_dPitchBendMod = dMod;}
	
	// --- for FM only (not used in Yamaha or my DX synths!)
	inline void setFoModLin(float dMod){m_dFoModLin = dMod;}

	// --- for Yamaha and my DX Synth
	inline void setPhaseMod(float dMod){m_dPhaseMod = dMod;}

	// --- PWM for square waves only
	inline void setPWMod(float dMod){m_dPWMod = dMod;}

	// --- VIRTUAL FUNCTIONS ----------------------------------------- //
	//
	// --- PURE ABSTRACT: derived class MUST implement
	// --- start/stop control
	virtual void startOscillator() = 0;
	virtual void stopOscillator() = 0;
	
	// --- render a sample
	//		for LFO:	 pAuxOutput = QuadPhaseOutput
	//			Pitched: pAuxOutput = Right channel (return value is left Channel
	float* pAuxOutput = NULL;
	virtual float doOscillate() = 0;
	float* pQuadPhaseOutput = NULL;
	// --- ABSTRACT: derived class overrides if needed
	virtual void setSampleRate(double* dFs){
		m_dSampleRate = dFs;
		m_dSampleRateF = float(*m_dSampleRate);
	}
	
	// --- reset counters, etc...
	virtual void reset();

	// INLINE FUNCTIONS: these are inlined because they will be 
	//                   called every sample period
	//					 You may want to move them to the .cpp file and
	//                   enable the compiler Optimization setting for 
	//					 Inline Function Expansion: Any Suitable though
	//					 inlining here forces it.
	//
	// --- init the global params
	inline void initGlobalParameters(globalOscillatorParams* pGlobalOscParams)
	{
		m_pGlobalOscParams = pGlobalOscParams;

		m_pGlobalOscParams->dOscFo = -1.0;	
		m_pGlobalOscParams->dFoRatio = m_dFoRatio;
		m_pGlobalOscParams->dAmplitude = m_dAmplitude;
		m_pGlobalOscParams->dwidth_float = m_dPulseWidthControl;
		m_pGlobalOscParams->nOctave = m_nOctave;
		m_pGlobalOscParams->nSemitones = m_nSemitones;
		m_pGlobalOscParams->nCents = m_nCents;
		m_pGlobalOscParams->uWaveform = m_uWaveform;
		m_pGlobalOscParams->uLFOMode = m_uLFOMode;
	}


	float pitchShiftFactor = 1.0;
	// --- update the frequency, amp mod and PWM
	inline void update(){		
		//m_dDelayTime = 1.0f;
		//m_dDelayCounter = 0.0;
		//increase 
		if (m_dDelayCounter < 1.0f) {
			m_dDelayCounter += m_dDelayInc;
			m_dFo = 0.0f;
		}
		else {
			m_dFo = m_dOscFo * m_dFoRatio*pitchShiftFactor;
		}
		//m_dFo = m_dOscFo * m_dFoRatio*pitchShiftFactor;
		
		// --- apply linear FM (not used in book projects)
		m_dFo += m_dFoModLin;
		// --- bound Fo (can go outside for FM/PM mod)
		//     +/- 20480 for FM/PM
		//m_dFo = fmax(fmin(m_dFo, OSC_FO_MAX), OSC_FO_MAX*-1);
		/*
		if (m_dFo > OSC_FO_MAX) {
			m_dFo = OSC_FO_MAX;
		}
		else if (m_dFo < -OSC_FO_MAX) {
			m_dFo = -OSC_FO_MAX;
		}
		*/
		//ParamClamp2(m_dFo, -OSC_FO_MAX, OSC_FO_MAX);
		
		// --- calculate increment (a.k.a. phase a.k.a. phaseIncrement, etc...)
		m_dInc = m_dFo/m_dSampleRateF;
	}
};
