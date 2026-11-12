#include "SEMFilter.h"
#include "pluginconstants.h"


CSEMFilter::CSEMFilter(void)
{
	// --- init
	m_dAlpha0 = 1.0;
	m_dAlpha = 1.0;
	m_dRho = 1.0;
	m_dAuxControl = 0.5; // for BSF this centers it
	m_dQControl = 1.0;
	//--- our default filter type
	m_uFilterType = _LPF;


	// --- flush registers
	reset();
}

CSEMFilter::~CSEMFilter(void)
{
}

// decode the Q value; Q on UI is 1->10
void CSEMFilter::setQControl(double dQControl)
{
	// this maps dQControl = 1->10 to Q = 0.5->25
	m_dQ = (25.0 - 0.5)*(dQControl - 1.0)/(10.0 - 1.0) + 0.5;
}

// recalc the coeffs


// do the filter

/*

double CSEMFilter::doFilter(double xn)
{
	// return xn if filter not supported
	

	// form the HP output first 
	endPoints[_HPF] = m_dAlpha0*(xn - m_dRho*m_dZ11 - m_dZ12);

	// BPF Out
	endPoints[_BPF] = m_dAlpha* endPoints[_HPF] + m_dZ11;

	// for nonlinear proc
	if(m_uNLP == ON)
		endPoints[_BPF] = fasttanh(m_dSaturation*endPoints[_BPF]);

	// LPF Out
	endPoints[_LPF] = m_dAlpha* endPoints[_BPF] + m_dZ12;

	// note R is the traditional analog damping factor
	//double R = 1.0/(2.0*m_dQ);

	// BSF Out
	//double bsf = xn - 2.0*R*bpf;

	// SEM BPF Output
	// using m_dAuxControl for this one-off control

	//double semBSF = m_dAuxControl*hpf + (1.0 - m_dAuxControl)*lpf;

	// update memory
	m_dZ11 = m_dAlpha* endPoints[_HPF] + endPoints[_BPF];
	m_dZ12 = m_dAlpha* endPoints[_BPF] + endPoints[_LPF];

	// return our selected type

	return endPoints[m_uFilterType];
	

	// return input if filter not supported
	//return xn;
}

*/