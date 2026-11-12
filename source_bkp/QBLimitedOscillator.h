#pragma once
#include "oscillator.h"
#include "RSine.h"

class CQBLimitedOscillator : public COscillator
{
public:
	CQBLimitedOscillator(void);
	~CQBLimitedOscillator(void);

	//int(CQBLimitedOscillator::*oscChoice[4])();
	void(CQBLimitedOscillator::*oscChoice[9])();

	// --- init globals
	inline virtual void initGlobalParameters(globalOscillatorParams* pGlobalOscParams)
	{
		// --- always call base class first to store pointer
		COscillator::initGlobalParameters(pGlobalOscParams);

		// --- add any QBL specifics here
		
	}

	// --- inline functions for realtime remdering stuff
	//

	int debugInt = 0;

	double dTrivialSaw;
	double dSwOut;
	bool fallingEdge = true;
	bool interpolation = false;
	void doSawtooth();


	// square with polyBLEP

	RSine rSine;
	void doRSine();

	double dSaw1;
	double dSaw2;
	double dSqOut;
	double dCorr;
	void doSquare();
	
	double triSample = 0.0;
	int triSign = 1;

	// DPW
	double doTriOut = 0.0;
	bool bDone;
	double dBipolar;
	double dSq;
	double dInv;
	double dSqMod;
	double dDifferentiatedSqMod;
	double c;
	void doTriangle();
	void doTriangle2();

	// -- virtual overrides
	virtual void reset();
	virtual void startOscillator();
	virtual void stopOscillator();

	void doSine();
	
	void doSaw1();

	void doSaw2();

	void doSaw3();

	void doNoise();

	void doPNoise();

	void postSawBlep();
	void postSquareBlep();
	void postTriBlep();

	void postBlep(bool fallingEdge);

	double dAngle;
	double doOscOut;
	bool bWrap;
	double dCalcModulo;
	// the rendering function
	virtual double doOscillate();


	inline double doSquareO(double dModulo, double dInc)
	{
		// --- sum-of-saws method
		//
		// --- pretend to be SAW1 type
		m_dPulseWidth = triCutoff;
		// --- get first sawtooth output
		double dSaw1 = doSawtoothO(dModulo, dInc);

		// --- phase shift on second oscillator
		if (dInc > 0)
			dModulo += m_dPulseWidth;
		else
			dModulo -= m_dPulseWidth;

		// --- for positive frequencies
		if (dInc > 0 && dModulo >= 1.0)
			dModulo -= 1.0;

		// --- for negative frequencies
		if (dInc < 0 && dModulo <= 0.0)
			dModulo += 1.0;

		// --- subtract saw method
		double dSaw2 = doSawtoothO(dModulo, dInc);

		// --- subtract = 180 out of phase
		double dOut = 0.5*dSaw1 - 0.5*dSaw2;

		// --- apply DC correction
		double dCorr = 1.0 / (m_dPulseWidth);

		// --- modfiy for less than 50%
		if ((m_dPulseWidth) < 0.5)
			dCorr = 1.0 / (1.0 - (m_dPulseWidth ));

		// --- apply correction
		dOut *= dCorr;

		// --- reset back to SQUARE
		m_uWaveform = SQUARE;

		return dOut;
	}

	inline double doSawtoothO(double dModulo, double dInc)
	{
		double dTrivialSaw = 0.0;
		double dOut = 0.0;

			dTrivialSaw = unipolarToBipolar(dModulo);
		
		// --- NOTE: Fs/8 = Nyquist/4
		if (m_dFo <= (*m_dSampleRate) / 8.0)
		{
			dOut = dTrivialSaw + doBLEP_N(&dBLEPTable_8_BLKHAR[0], /* BLEP table */
				4096,			/* BLEP table length */
				dModulo,		/* current phase value */
				fabs(dInc),	/* abs(dInc) is for FM synthesis with negative frequencies */
				1.0,			/* sawtooth edge height = 1.0 */
				false,		/* falling edge */
				4,			/* 1 point per side */
				false);		/* no interpolation */
		}
		else // to prevent overlapping BLEPs, default back to 2-point for f > Nyquist/4
		{
			dOut = dTrivialSaw + doBLEP_N(&dBLEPTable[0], /* BLEP table */
				4096,			/* BLEP table length */
				dModulo,		/* current phase value */
				fabs(dInc),	/* abs(dInc) is for FM synthesis with negative frequencies */
				1.0,			/* sawtooth edge height = 1.0 */
				false,		/* falling edge */
				1,			/* 1 point per side */
				false);		/* no interpolation */
		}

		// --- or do PolyBLEP
		//dOut = dTrivialSaw + doPolyBLEP_2(dModulo, 
		//								  abs(dInc),/* abs(dInc) is for FM synthesis with negative frequencies */
		//								  1.0,		/* sawtooth edge = 1.0 */
		//								  false);	/* falling edge */

		return dOut;
	}

	inline double doBlep(double dModulo, double doOscOut, double dInc, bool fallingE)
	{
		double dOut = 0.0;

		// --- NOTE: Fs/8 = Nyquist/4
		if (m_dFo <= (*m_dSampleRate) / 8.0)
		{
			dOut = doOscOut + doBLEP_N(&dBLEPTable_8_BLKHAR[0], /* BLEP table */
				4096,			/* BLEP table length */
				dModulo,		/* current phase value */
				fabs(dInc),	/* abs(dInc) is for FM synthesis with negative frequencies */
				2.0,			/* sawtooth edge height = 1.0 */
				fallingE,		/* falling edge */
				4,			/* 1 point per side */
				false);		/* no interpolation */
		}
		else // to prevent overlapping BLEPs, default back to 2-point for f > Nyquist/4
		{
			dOut = doOscOut + doBLEP_N(&dBLEPTable[0], /* BLEP table */
				4096,			/* BLEP table length */
				dModulo,		/* current phase value */
				fabs(dInc),	/* abs(dInc) is for FM synthesis with negative frequencies */
				2.0,			/* sawtooth edge height = 1.0 */
				fallingE,		/* falling edge */
				1,			/* 1 point per side */
				false);		/* no interpolation */
		}

		// --- or do PolyBLEP
		//dOut = dTrivialSaw + doPolyBLEP_2(dModulo, 
		//								  abs(dInc),/* abs(dInc) is for FM synthesis with negative frequencies */
		//								  1.0,		/* sawtooth edge = 1.0 */
		//								  false);	/* falling edge */

		return dOut;
	}
};
