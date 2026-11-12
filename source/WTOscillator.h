#pragma once
#include "oscillator.h"



class CWTOscillatorVoice2 : public COscillator
{
public:
	CWTOscillatorVoice2(void);
	~CWTOscillatorVoice2(void);

	
	
	
	// main oscillator
	double m_dReadIndex;
	double m_dWT_inc;
	
	// unisom mode buddy
	double m_dReadIndex2;
	double m_dWT_inc2;
	
	// our tables

	//double(*ptr)[WT_LENGTH];
	double m_dSineTable[WT_LENGTH];
	double* m_pSawTables[NUM_TABLES];
	double* m_pTriangleTables[NUM_TABLES];

	// for storing current table
	double* m_pCurrentTable;
	int m_nCurrentTableIndex; //0 - 9

	// correction factor table sum-of-sawtooth
	double m_dSquareCorrFactor[NUM_TABLES];

	// find the table with the proper number of harmonics for our pitch
	int getTableIndex();
	void selectTable();

	// create an destory tables
	void createWaveTables();
	void destroyWaveTables();

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
	virtual double doOscillate(double* pAuxOutput);

	// wave table specific
	virtual void setSampleRate(double* dFs);
	virtual void update();
};
