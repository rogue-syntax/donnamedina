#include "MoogLadderFilter.h"

CMoogLadderFilter::CMoogLadderFilter(void)
{
	// init
	m_dK = 0.0;
	m_dAlpha_0 = 1.0;
	m_dA = 0.0;
	m_dB = 0.0;
	m_dC = 0.0;
	m_dD = 0.0;
	
	// set all as LPF types
	m_LPF1.m_uFilterType = LPF1;
	m_LPF2.m_uFilterType = LPF1;
	m_LPF3.m_uFilterType = LPF1;
	//m_LPF4.m_uFilterType = LPF1;

	// set default filter type
	m_uFilterType = LPF4;
	//m_uNLP = 1;
	// flush everything
	reset();
}

CMoogLadderFilter::~CMoogLadderFilter(void)
{
}

void CMoogLadderFilter::reset()
{
	// flush everything
	m_LPF1.reset();
	m_LPF2.reset();
	//m_LPF3.reset();
	//m_LPF4.reset();
}

// decode the Q value; Q on UI is 1->10
void CMoogLadderFilter::setQControl(double dQControl)
{
	// this maps dQControl = 1->10 to K = 0 -> 4
	m_dK = (4.0)*(dQControl - 1.0)/(10.0 - 1.0);
}

void CMoogLadderFilter::update()
{
	// do any modulation first
	//CFilter::update();
	setQControl(m_dQControl);
	m_dFc = fmax(fmin(m_dFcControl, FILTER_FC_MAX), FILTER_FC_MIN);
	//m_dFc = m_dFcControl;
	// prewarp for BZT
	
	wd = 2 * pi*m_dFc;
	//wd = 2 * pi*m_dFcControl;
	T = 1 / m_dSampleRate;
	wa = (2 / T)*tan(wd*T / 2);
	g = wa * T / 2;

	// G - the feedforward coeff in the VA One Pole
	//     same for LPF, HPF
	G = g / (1.0 + g);

	// set alphas
	//CHANGED FROM 4 POLE TO THREE POLE
	//BY ROGE SYNTAX
	//STILL TESTING
	m_LPF1.m_dAlpha = G;
	m_LPF2.m_dAlpha = G;
	m_LPF3.m_dAlpha = G;
	//m_LPF4.m_dAlpha = G;

	// set betas
	/*
	m_LPF1.m_dBeta = G*G*G/(1.0 + g);
	m_LPF2.m_dBeta = G*G/(1.0 + g);
	m_LPF3.m_dBeta = G/(1.0 + g);
	m_LPF4.m_dBeta = 1.0/(1.0 + g);
	*/

	m_LPF1.m_dBeta = G  / (1.0 + g);
	m_LPF2.m_dBeta = 1.0 / (1.0 + g);
	//m_LPF3.m_dBeta = 1.0 / (1.0 + g);

	//m_dGamma = G*G*G*G; // G^4
	m_dGamma = G * G; // G^2

	m_dAlpha_0 = 1.0 / (1.0 + m_dK * m_dGamma);

	//using LP4 only
	m_dA = 0.0;
	m_dB = 0.0;
	m_dC = 1.0;
	m_dD = 1.0;
	m_dE = 1.0;

	// Oberhein variation
	//REMOVED BY ROGUE SYNTAX 2019
	/*
	switch(m_uFilterType)
	{
		case LPF4:
			m_dA = 0.0;
			m_dB = 0.0;
			m_dC = 0.0;
			m_dD = 0.0;
			m_dE = 1.0;
			break;

		case LPF2:
			m_dA = 0.0;
			m_dB = 0.0;
			m_dC = 1.0;
			m_dD = 0.0;
			m_dE = 0.0;
			break;

		case BPF4:
			m_dA = 0.0;
			m_dB = 0.0;
			m_dC = 4.0;
			m_dD = -8.0;
			m_dE = 4.0;
			break;

		case BPF2:
			m_dA = 0.0;
			m_dB = 2.0;
			m_dC = -2.0;
			m_dD = 0.0;
			m_dE = 0.0;
			break;

		case HPF4:
			m_dA = 1.0;
			m_dB = -4.0;
			m_dC = 6.0;
			m_dD = -4.0;
			m_dE = 1.0;
			break;

		case HPF2:
			m_dA = 1.0;
			m_dB = -2.0;
			m_dC = 1.0;
			m_dD = 0.0;
			m_dE = 0.0;
			break;

		default: // LPF4
			m_dA = 0.0;
			m_dB = 0.0;
			m_dC = 0.0;
			m_dD = 0.0;
			m_dE = 1.0;
			break;

	}
	*/
}

double CMoogLadderFilter::doFilter(double xn)
{
	// --- return xn if filter not supported
	//REMOVED BY ROGUE SYNTAX 2019
	/*
	if(m_uFilterType == BSF2 || m_uFilterType == LPF1 || m_uFilterType == HPF1)
		return xn;
	*/

	dSigma = m_LPF1.getFeedbackOutput() +
		m_LPF2.getFeedbackOutput();
		//+ m_LPF3.getFeedbackOutput();
	//+ m_LPF4.getFeedbackOutput();

// --- for passband gain compensation
//     you can connect this to a GUI control 
//     and let user choose instead
	xn *= 1.0 + m_dAuxControl * m_dK;

	// calculate input to first filter
	dU = (xn - m_dK * dSigma)*m_dAlpha_0;

	//if (m_uNLP == ON)
	//	dU = fasttanh(m_dSaturation*dU);

	// --- cascade of 4 filters
	dLP1 = m_LPF1.doFilter(dU);
	dLP2 = m_LPF2.doFilter(dLP1);
	//dLP3 = m_LPF3.doFilter(dLP2);
	//double dLP4 = m_LPF4.doFilter(dLP3);

	// --- Oberheim variations
	//return m_dA*dU + m_dB*dLP1 + m_dC*dLP2 + m_dD*dLP3 +  m_dE*dLP4;  
	//return m_dA * dU + m_dB * dLP1 + m_dC * dLP2 + m_dD * dLP3;
	return m_dA * dU + m_dB * dLP1 + m_dC * dLP2;
}


void CMoogLadderFilter::applyFilter(float* xn)
{
	// --- return xn if filter not supported
	//REMOVED BY ROGUE SYNTAX 2019
	/*
	if(m_uFilterType == BSF2 || m_uFilterType == LPF1 || m_uFilterType == HPF1)
		return xn;
	*/

	dSigma = m_LPF1.getFeedbackOutput() +
		m_LPF2.getFeedbackOutput();
		//m_LPF3.getFeedbackOutput();
	//+ m_LPF4.getFeedbackOutput();

// --- for passband gain compensation
//     you can connect this to a GUI control 
//     and let user choose instead
	(*xn) *= static_cast<float>(1.0 + m_dAuxControl * m_dK);

	// calculate input to first filter
	dU = ((*xn) - m_dK * dSigma)*m_dAlpha_0;

	//if (m_uNLP == ON)
	//dU = fasttanh(m_dSaturation*dU);

	// --- cascade of 4 filters
	dLP1 = m_LPF1.doFilter(dU);
	dLP2 = m_LPF2.doFilter(dLP1);
	//dLP3 = m_LPF3.doFilter(dLP2);
	//double dLP4 = m_LPF4.doFilter(dLP3);

	// --- Oberheim variations
	//return m_dA*dU + m_dB*dLP1 + m_dC*dLP2 + m_dD*dLP3 +  m_dE*dLP4;  
	(*xn) = static_cast<float>(m_dA * dU + m_dB * dLP1 + m_dC * dLP2);
	//return m_dA * dU + m_dB * dLP1 + m_dC * dLP2 + m_dD * dLP3;
}

float CMoogLadderFilter::fasttanh(float p)
{
	//  return -1.0f + 2.0f / (1.0f + fastexp (-2.0f * p));
	return static_cast<float>( p / (fabs(2 * p) + 3 / (2 + 2 * p * 2 * p)) );
}
