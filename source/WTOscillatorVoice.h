
#pragma once
#include "oscillator.h"
#include "WTTables.h"




class CWTOscillatorVoice : public COscillator
{
public:
	CWTOscillatorVoice(void);
	void setSampleRate(double* dFs);
	~CWTOscillatorVoice(void);

	
	
	// main oscillator
	double m_dReadIndex;
	double m_dWT_inc;
	
	// unisom mode buddy
	double m_dReadIndex2;
	double m_dWT_inc2;
	
	// our tables
	//instead hold pointer to a WTTables obj to use its tables

	CWTTables wTTables;

	//double m_dSineTable[WT_LENGTH];
	//double* m_pSawTables[NUM_TABLES];
	//double* m_pTriangleTables[NUM_TABLES];
	// for storing current table
	double* m_pCurrentTable;
	int m_nCurrentTableIndex; //0 - 9

	// correction factor table sum-of-sawtooth
	double m_dSquareCorrFactor[NUM_TABLES];

	// find the table with the proper number of harmonics for our pitch
	int getTableIndex();
	void selectTable();

	// create an destory tables
	

	// do the selected wavetable
	double doWaveTable(double& dReadIndex, double dWT_inc);

	// for square wave we have more work to do
	double doSquareWave();
	double doSquareWaveCore(double& dReadIndex, double dWT_inc);


	inline void checkWrapIndex(double& dIndex)
	{
		while(dIndex < 0.0)
			dIndex += WT_LENGTH;

		while(dIndex >= WT_LENGTH)
			dIndex -= WT_LENGTH;
	}

	// typical overrides
	virtual void reset();
	virtual void startOscillator();
	virtual void stopOscillator();
	// render a sample
	// for LFO:		pAuxOutput = QuadPhaseOutput
	//     Pitched: pAuxOutput = Right channel (return value is left Channel
	virtual double doOscillate();

	// wave table specific
	
	virtual void update();

	
	
	int(CWTOscillatorVoice::*tableChoice[5])();

	int selectSaw();
	int selectTri();
	int selectNone();



};

typedef void(CWTOscillatorVoice::*TableChoices)();
