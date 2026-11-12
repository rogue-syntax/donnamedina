#pragma once
#include "filter.h"
#include "synthfunctions.h"

class CSEMFilter : public CFilter
{
public:
	CSEMFilter(void);
	~CSEMFilter(void);

	// --- Trapezoidal Integrator Components
	double m_dAlpha0;		// input scalar
	double m_dAlpha;		// alpha is same as VA One Pole
	double m_dRho;			// feedback
	float rawCtrl;
	double wd;
	double T;
	double wa;
	double g;
	double R;
	double lpf;
	double bpf;
	double hpf;
	//butterw
	float Wc; // cutoff frequency in rad/s
	float K;
	float norm;
	float a0;
	float a1;
	float a2;
	float a3;
	float z1;
	float z2;
	float z3;
	float p0;
	float p1;
	float p2;
	float b1;
	float b2;
	float b3 = (K*K*K - 2 * K*K + 2 * K - 1) * norm;
	float input;

	void updateButterworth() {
		Wc = m_dFcControl; // cutoff frequency in rad/s
		K = std::tan(pi * Wc);
		norm = 1 / (K*K*K + 2 * K*K + 2 * K + 1);
		a0 = K * K*K*norm;
		a1 = 3 * a0;
		a2 = a1;
		a3 = a0;
		b1 = (3 * K*K*K + 2 * K*K - 2 * K - 3) * norm;
		b3 = (K*K*K - 2 * K*K + 2 * K - 1) * norm;
	}
	

	void doButterworth(float& output) {
		input = output;
		output = output * a0 + z1;
		z1 = input * a1 + z2 - b1 * output;
		z2 = input * a2 + z3 - b2 * output;
		z3 = input * a3 - b3 * output;
		p0 = p1; p1 = p2; p2 = input;

		// Since LPF is not stable on first N frame
		// 1) we need to bypass input at first N-3 frames
		// 2) cache input between [N-3, N), no output at all
		// 3) between [N, N+3), we need to blend the output with the cached inputs with index based weights
		// 4) for following frames, we can use the LPF normally
	}
	// -- CFilter Overrides --
	virtual void reset(){
		m_dZ11 = 0; 
		m_dZ12 = 0.0;
		T = 1 / m_dSampleRate;
	}

	virtual void setQControl(double dQControl);
	


	inline void update(){
		m_dFc = m_dFcControl;
		g = ((2 / T)*tan((2 * pi*m_dFc)*T / 2))*(T / 2);
		R = 1.0 / (2.0*m_dQ);
		m_dAlpha0 = 1.0 / (1.0 + 2.0*R*g + g * g);
		m_dAlpha = g;
		m_dRho = 2.0*R + g;
	}

	// Optimization: the voice looks g up in a table built with gFor (the expensive tan()) and sets
	// it with setG (cheap). Same formulas as update().
	inline double gFor(double fc) const { return ((2 / T)*tan((2 * pi*fc)*T / 2))*(T / 2); }
	inline double sampleInterval() const { return T; }
	inline void setG(double gg) {
		g = gg;
		R = 1.0 / (2.0*m_dQ);
		m_dAlpha0 = 1.0 / (1.0 + 2.0*R*g + g * g);
		m_dAlpha = g;
		m_dRho = 2.0*R + g;
	}

	inline double doFilter(double xn) {

		// form the HP output first 
		endPoints[_HPF] = m_dAlpha0 * (xn - m_dRho * m_dZ11 - m_dZ12);

		// BPF Out
		endPoints[_BPF] = m_dAlpha * endPoints[_HPF] + m_dZ11;

		// for nonlinear proc
		//if (m_uNLP == ON)
			//endPoints[_BPF] = fasttanh(m_dSaturation*endPoints[_BPF]);

		// LPF Out
		endPoints[_LPF] = m_dAlpha * endPoints[_BPF] + m_dZ12;

		// note R is the traditional analog damping factor
		//double R = 1.0/(2.0*m_dQ);

		// BSF Out
		//double bsf = xn - 2.0*R*bpf;

		// SEM BPF Output
		// using m_dAuxControl for this one-off control

		//double semBSF = m_dAuxControl*hpf + (1.0 - m_dAuxControl)*lpf;

		// update memory
		m_dZ11 = m_dAlpha * endPoints[_HPF] + endPoints[_BPF];
		m_dZ12 = m_dAlpha * endPoints[_BPF] + endPoints[_LPF];

		// return our selected type

		return endPoints[m_uFilterType];

	};

	inline void doFilter2(float &xn) { 
		double aaa = m_dAlpha0 * (xn - m_dRho * m_dZ11 - m_dZ12);
		double bbb = m_dAlpha * aaa + m_dZ11;
		double ccc = m_dAlpha * bbb + m_dZ12;
		m_dZ11 = m_dAlpha * aaa + bbb;
		m_dZ12 = m_dAlpha * bbb + ccc;
		xn = ccc;
	};


protected:
	double m_dZ11;		// our z-1 storage location
	double m_dZ12;		// our z-1 storage location # 2

	//int(CSEMFilter::*selectFilter[3])(double xn);
	double endPoints[3];
	
};
