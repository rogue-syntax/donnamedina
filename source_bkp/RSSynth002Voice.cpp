#include "RSSynth002Voice.h"

CRSSynth002Voice::CRSSynth002Voice(void)
{
	
}


CRSSynth002Voice::~CRSSynth002Voice(void)
{
}

void CRSSynth002Voice::setSampleRate(double dSampleRate)
{
	CVoice::setSampleRate(dSampleRate);
}

void CRSSynth002Voice::prepareForPlay()
{
	CVoice::prepareForPlay();
	reset();
}

void CRSSynth002Voice::reset()
{
	CVoice::reset();
	
}

void CRSSynth002Voice::update()
{
	// --- voice specific updates
	if(!m_pGlobalVoiceParams) return;

	// --- always call base class first
	CVoice::update();

	
}