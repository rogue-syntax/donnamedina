
#ifndef __vst_synth_processor__
#define __vst_synth_processor__

#include "public.sdk/source/vst/vst2wrapper/vst2wrapper.h"
#include "vstgui/vstgui.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstevents.h"

#include "RSSynth002Voice.h"

#define MAX_VOICES 16
#define OUTPUT_CHANNELS 2 // stereo only!

namespace Steinberg {
namespace Vst {
class VoiceProcessor;

namespace rs {

class Processor : public AudioEffect
{
public:
	Processor();
	~Processor();

	tresult PLUGIN_API initialize(FUnknown* context);
	tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts);
	tresult PLUGIN_API setState(IBStream* fileStream);
	tresult PLUGIN_API getState(IBStream* fileStream);
	tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize);
	tresult PLUGIN_API setActive(TBool state);
	tresult PLUGIN_API process(ProcessData& data);
	static FUnknown* createInstance (void*) { return (IAudioProcessor*)new Processor (); }
	static FUID cid;
protected:
	CRSSynth002Voice* m_pVoiceArray[MAX_VOICES];
	CModulationMatrix m_GlobalModMatrix;
	globalSynthParams m_GlobalSynthParams;
	void incrementVoiceTimestamps();
	CRSSynth002Voice* getOldestVoice();
	CRSSynth002Voice* getOldestVoiceWithNote(UINT uMIDINote);
	int getNoteOnCount();
	bool doControlUpdate(ProcessData& data);
	bool doProcessEvent(Event& vstEvent);
	void update();
	double m_dLastNoteFrequency;
	UINT m_uMidiRxChannel;
	UINT m_uVoiceMode;
	double m_dVolume_dB;
	double m_dMIDIPitchBend; 
	UINT m_uMIDIModWheel;
	UINT m_uMIDIVolumeCC7;
	UINT m_uMIDIPanCC10;
	UINT m_uMIDIExpressionCC11;
	#if defined _WINDOWS || defined _WINDLL
	char* getMyDLLDirectory(UString cPluginName)
	{
		// ger our handle
		HMODULE hmodule = GetModuleHandle(cPluginName);

		// an array to hold the directory
		TCHAR dir[MAX_PATH];
		memset(&dir[0], 0, MAX_PATH*sizeof(TCHAR));
		dir[MAX_PATH-1] = '\0';

		// get the path to the module
		if(hmodule)
			GetModuleFileName(hmodule, &dir[0], MAX_PATH);
		else
			return NULL;

		// convert to UString
		UString DLLPath(&dir[0], MAX_PATH);

		// path strings
		char* pFullPath = new char[MAX_PATH];
		char* pDLLRoot = new char[MAX_PATH];

		// convert USTRING to char*
		DLLPath.toAscii(pFullPath, MAX_PATH);

		// copy over the path MINUS the filename and backslash
		int nLenDir = strlen(pFullPath);
		int nLenDLL = wcslen(cPluginName) + 1;	// +1 is for trailing backslash
		memcpy(pDLLRoot, pFullPath, nLenDir-nLenDLL);

		// set trailing null char
		pDLLRoot[nLenDir-nLenDLL] = '\0';

		// clean up
		delete [] pFullPath;

		// caller must delete this after use
		return pDLLRoot;
	}
	#endif
	


};

}}} // namespaces

#endif
