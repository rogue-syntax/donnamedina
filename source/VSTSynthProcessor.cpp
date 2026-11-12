
#include "VSTSynthProcessor.h"
#include "VSTSynthController.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/base/futils.h"


#include "synthparamlimits.h"

#define SYNTH_PROC_BLOCKSIZE 64 // 32 samples per processing block = 0.7 mSec = OK for tactile response WP

::AudioEffect* createEffectInstance(audioMasterCallback audioMaster){
	return Steinberg::Vst::Vst2Wrapper::create(GetPluginFactory(), Steinberg::Vst::rs::Processor::cid,	'DXS0',	audioMaster);
}

namespace Steinberg {
namespace Vst {
namespace rs {

// --- for versioning in serialization
static uint64 RSSynth002Version = 0;
FUID Processor::cid (0x082D00C8, 0xBDCC4D4D, 0x9B94A536, 0x16FB8193);


Processor::Processor (){
	// --- we are a Processor
	setControllerClass(Controller::cid);

	// our inits
	m_uVoiceMode = DEFAULT_VOICE_MODE;
	m_dVolume_dB = DEFAULT_OUTPUT_AMPLITUDE_DB;
	
	// Finish initializations here
	m_dLastNoteFrequency = 0.0;

	// receive on all channels
	m_uMidiRxChannel = MIDI_CH_ALL;

	// load up voices
	for(int i=0; i<MAX_VOICES; i++){
		// --- create voice
		m_pVoiceArray[i] = new CRSSynth002Voice;

		// --- global params (MUST BE DONE before setting up mod matrix!)
		m_pVoiceArray[i]->initGlobalParameters(&m_GlobalSynthParams);
	}


}

// --- destructor
Processor::~Processor()
{
	// --- delete voices
	for(int i=0; i<MAX_VOICES; i++)
	{
		if(m_pVoiceArray[i])
		{
			delete m_pVoiceArray[i];
			m_pVoiceArray[i] = NULL;
		}
	}
}

/*
	Processor::initialize()
	Call the base class
	Add a Stereo Audio Output
	Add a MIDI event inputs (16: one for each channel)
*/
tresult PLUGIN_API Processor::initialize(FUnknown* context)
{
	tresult result = AudioEffect::initialize(context);
	if (result == kResultTrue)
	{
		// stereo output bus
		addAudioOutput(STR16 ("Audio Output"), SpeakerArr::kStereo);

		// MIDI event input bus, one channel
		addEventInput(STR16 ("Event Input"), 16);
	}

	return result;
}

/*
	Processor::getState()
	This is the READ part of the serialization process. We get the stream interface and use it
	to read from the filestream.

	NOTE: The datatypes/read order must EXACTLY match the getState() version or crashes may happen or variables
	      not initialized properly.
*/
tresult PLUGIN_API Processor::setState(IBStream* fileStream)
{
	IBStreamer s(fileStream, kLittleEndian);
	uint64 version = 0;

	// --- needed to convert to our UINT reads
	uint32 udata = 0;
	int32 data = 0;

	//// read the version
	if(!s.readInt64u(version)) return kResultFalse;

	//// --- then read Version 0 params
	if(!s.readInt32u(udata)) return kResultFalse; else m_uVoiceMode = udata;
	if (!s.readDouble(m_dVolume_dB)) return kResultFalse;

	

	return kResultTrue;
}

/*
	Processor::getState()
	This is the WRITE part of the serialization process. We get the stream interface and use it
	to write to the filestream. This is important because it is how the Factory Default is set
	at startup, as well as when writing presets.
*/
tresult PLUGIN_API Processor::getState(IBStream* fileStream)
{
	// get a stream I/F
	IBStreamer s(fileStream, kLittleEndian);

	// --- VectorSynthVersion - place this at top so versioning can be used during the READ operation
	if(!s.writeInt64u(RSSynth002Version)) return kResultFalse;

	if(!s.writeInt32u(m_uVoiceMode)) return kResultFalse;
	if (!s.writeDouble(m_dVolume_dB)) return kResultFalse;
	
	return kResultTrue;
}

/*
	Processor::setBusArrangements()
	Client queries us for our supported Busses; this is where you can modify to support mono, surround, etc...
*/
tresult PLUGIN_API Processor::setBusArrangements (SpeakerArrangement* inputs, int32 numIns,
															SpeakerArrangement* outputs, int32 numOuts)
{
	// we only support one stereo output bus
	if (numIns == 0 && numOuts == 1 && outputs[0] == SpeakerArr::kStereo)
	{
		return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
	}
	return kResultFalse;
}

/*
	Processor::canProcessSampleSize()
	Client queries us for our supported sample lengths
*/
tresult PLUGIN_API Processor::canProcessSampleSize(int32 symbolicSampleSize)
{
	// this is a challenge in the book; here is where you say you support it but
	// you will need to deal with different buffers in the process() method
//	if (symbolicSampleSize == kSample32 || symbolicSampleSize == kSample64)

	// --- currently 32 bit only
	if (symbolicSampleSize == kSample32 || symbolicSampleSize == kSample64)
	{
		return kResultTrue;
	}
	return kResultFalse;
}


/*
	Processor::setActive()
	This is the analog of prepareForPlay() in RAFX since the Sample Rate is now set.

	VST3 plugins may be turned on or off; you are supposed to dynamically delare stuff when activated
	then delete when de-activated. So, we handle our voice stack here.
*/
tresult PLUGIN_API Processor::setActive(TBool state)
{
	if(state)
	{
		// --- reset
		m_dLastNoteFrequency = 0.0;

		// --- set sample rate; it is now valid (could have changed?)
		for(int i=0; i<MAX_VOICES; i++)
		{
			if(m_pVoiceArray[i])
			{
				m_pVoiceArray[i]->setSampleRate((double)processSetup.sampleRate);
				m_pVoiceArray[i]->prepareForPlay();
			}
		}

		// --- mass update 
		update();
	}
	else
	{
		// --- for any customization you may do
	}

	// base class method call is last
	return AudioEffect::setActive (state);
}


/*
	Processor::update()
	Our own function to update the voice(s) of the synth with UI Changes.
*/
void Processor::update()
{
	// --- update global parameters
	//
	// --- Voice:
	// for FM synth, Voice Mode = FM Algorithm
	//m_GlobalSynthParams.voiceParams.uVoiceMode = m_uVoiceMode;
	
}


// --- increment the timestamp for new note events
void Processor::incrementVoiceTimestamps()
{
	for(int i=0; i<MAX_VOICES; i++)
	{
		if(m_pVoiceArray[i])
		{
			// if on, inc
			if(m_pVoiceArray[i]->m_bNoteOn)
				m_pVoiceArray[i]->m_uTimeStamp++;
		}
	}
}

// --- get oldest note
CRSSynth002Voice* Processor::getOldestVoice()
{
	int nTimeStamp = -1;
	CRSSynth002Voice* pFoundVoice = NULL;
	for(int i=0; i<MAX_VOICES; i++)
	{
		if(m_pVoiceArray[i])
		{
			// if on and older, save
			// highest timestamp is oldest
			if(m_pVoiceArray[i]->m_bNoteOn && (int)m_pVoiceArray[i]->m_uTimeStamp > nTimeStamp)
			{
				pFoundVoice = m_pVoiceArray[i];
				nTimeStamp = (int)m_pVoiceArray[i]->m_uTimeStamp;
			}
		}
	}
	return pFoundVoice;
}

// --- get oldest voice with a MIDI note also
CRSSynth002Voice* Processor::getOldestVoiceWithNote(UINT uMIDINote)
{
	int nTimeStamp = -1;
	CRSSynth002Voice* pFoundVoice = NULL;
	for(int i=0; i<MAX_VOICES; i++)
	{
		if(m_pVoiceArray[i])
		{
			// if on and older and same MIDI note, save
			// highest timestamp is oldest
			if(m_pVoiceArray[i]->canNoteOff() &&
				(int)m_pVoiceArray[i]->m_uTimeStamp > nTimeStamp &&
				m_pVoiceArray[i]->m_uMIDINoteNumber == uMIDINote)
			{
				pFoundVoice = m_pVoiceArray[i];
				nTimeStamp = (int)m_pVoiceArray[i]->m_uTimeStamp;
			}
		}
	}
	return pFoundVoice;
}

/*
	Processor::getNoteCount()
	returns number of notes currently playing
*/
int Processor::getNoteOnCount()
{
	int nCount = 0;
	for(int i=0; i<MAX_VOICES; i++)
	{
		if(m_pVoiceArray[i])
		{
			if(m_pVoiceArray[i]->m_bNoteOn)
				nCount++;
		}
	}

	return nCount;
}

/*
	Processor::doControlUpdate()
	Find and issue Control Changes (same as userInterfaceChange() in RAFX)
	returns true if a control was changed
*/
bool Processor::doControlUpdate(ProcessData& data)
{
	bool paramChange = false;

	// --- check
	if(!data.inputParameterChanges)
	return paramChange;

	// --- get the param count and setup a loop for processing queue data
	//FIGURE THIS OUTB4 HAND FOO
	int32 count = data.inputParameterChanges->getParameterCount();

	// --- make sure there is something there
	if(count <= 0)
	return paramChange;

	// --- loop
	for(int32 i=0; i<count; i++)
	{
		// get the message queue for ith parameter
		IParamValueQueue* queue = data.inputParameterChanges->getParameterData(i);

		if(queue)
		{
			// --- check for control points
			if(queue->getPointCount() <= 0) return false;

			int32 sampleOffset = 0.0;
			ParamValue value = 0.0;
			ParamID pid = queue->getParameterId();

			// this is the same as userInterfaceChange(); these only are updated if a change has
			// occurred (a control got moved)
			//
			// NOTE: same as with VST sample synth, we are taking the last value in the queue: queue->getPointCount()-1
			//       the value parameter is [0..1] so MUST BE COOKED before using
			//
			// TODO: maybe try to make this nearly sample accurate <- from Steinberg...

			// --- get the last point in queue
			if(queue->getPoint(queue->getPointCount()-1, /* last update point */
				sampleOffset,			/* sample offset */
				value) == kResultTrue)	/* value = [0..1] */
			{
				// --- at least one param changed
				paramChange = true;

				switch (pid) // same as RAFX uControlID
				{
					case VOICE_MODE:
					{
						//avoid branchine by doing something like this:
						//eventFuncs[voice_mode] = void function( value ){ m_uVoiceMode = (UINT)cookVSTGUIVariable(MIN_VOICE_MODE, MAX_VOICE_MODE, value); }

						// cookVSTGUIVariable(min, max, currentValue) <- cooks raw data into meaningful info for us
						m_uVoiceMode = (UINT)cookVSTGUIVariable(MIN_VOICE_MODE, MAX_VOICE_MODE, value);
						break;
					}

					case OUTPUT_AMPLITUDE_DB:
					{
						m_dVolume_dB = cookVSTGUIVariable(MIN_OUTPUT_AMPLITUDE_DB, MAX_OUTPUT_AMPLITUDE_DB, value);
						break;
					}

					
				}
			}
		}
	}

	// --- check and update
	if(paramChange)
		update();

	return paramChange;
}

bool Processor::doProcessEvent(Event& vstEvent)
{
	bool noteEvent = false;
	// --- process Note On or Note Off messages here; these are individual functions in RAFX
	switch(vstEvent.type)
	{
		// --- NOTE ON
		case Event::kNoteOnEvent:
		{
			// --- get the channel/note/vel
			UINT uMIDIChannel = (UINT)vstEvent.noteOn.channel;
			UINT uMIDINote = (UINT)vstEvent.noteOn.pitch;
			UINT uVelocity = (UINT)(127.0*vstEvent.noteOn.velocity);

			// --- test channel/ignore
			if(m_uMidiRxChannel != MIDI_CH_ALL && uMIDIChannel != m_uMidiRxChannel)
				return false;

			// --- event occurred
			noteEvent = true;

			// --- fix noteID as per SDK
			if(vstEvent.noteOn.noteId == -1)
				vstEvent.noteOn.noteId = uMIDINote;

			bool bStealNote = true;
			for(int i=0; i<MAX_VOICES; i++)
			{
				// --- loop and find free voice
				if(m_pVoiceArray[i])
				{
					// -- if we have a free voice, turn on
					if(!m_pVoiceArray[i]->m_bNoteOn)
					{
						// --- do this first
						incrementVoiceTimestamps();

						// --- then note on
						m_pVoiceArray[i]->noteOn(uMIDINote, uVelocity, midiFreqTable[uMIDINote], m_dLastNoteFrequency);

						//#if(LOG_MIDI && _DEBUG)
							//FDebugPrint("-- Note On Ch:%d Note:%d Vel:%d \n", uMIDIChannel, uMIDINote, uVelocity);
						//#endif

						// --- save
						m_dLastNoteFrequency = midiFreqTable[uMIDINote];
						bStealNote = false;
						break;
					}
				}
			}

			// --- need to steal
			if(bStealNote)
			{
				// --- steal oldest note
				CRSSynth002Voice* pVoice = getOldestVoice();
				if(pVoice)
				{
					// --- do this first
					incrementVoiceTimestamps();

					// --- then note on
					pVoice->noteOn(uMIDINote, uVelocity, midiFreqTable[uMIDINote],
							   m_dLastNoteFrequency);

					#if(LOG_MIDI && _DEBUG)
						FDebugPrint("-- Note Stolen! Ch:%d Note:%d Vel:%d \n", uMIDIChannel, uMIDINote, uVelocity);
					#endif
				}

				// --- save
				m_dLastNoteFrequency = midiFreqTable[uMIDINote];
			}

			break;
		}

		// --- NOTE OFF
		case Event::kNoteOffEvent:
		{
			// --- get the channel/note/vel
			UINT uMIDIChannel = (UINT)vstEvent.noteOff.channel;
			UINT uMIDINote = (UINT)vstEvent.noteOff.pitch;
			UINT uVelocity = (UINT)(127.0*vstEvent.noteOff.velocity); // not used

			// --- test channel/ignore
			if(m_uMidiRxChannel != MIDI_CH_ALL && uMIDIChannel != m_uMidiRxChannel)
				return false;

			// --- event occurred
			noteEvent = true;

			// --- fix noteID as per SDK
			if(vstEvent.noteOff.noteId == -1)
				vstEvent.noteOff.noteId = uMIDINote;

			for(int i=0; i<MAX_VOICES; i++)
			{
				CRSSynth002Voice* pVoice = getOldestVoiceWithNote(uMIDINote);
				if(pVoice)
				{
					pVoice->noteOff(uMIDINote);
				#if(LOG_MIDI && _DEBUG)
					FDebugPrint("-- Note Off Ch:%d Note:%d Vel:%d \n", uMIDIChannel, uMIDINote, uVelocity);
				#endif
					break;
				}
			}

			break;
		}
		// --- polyphonic aftertouch 0xAn
		case Event::kPolyPressureEvent:
		{
			// --- get the channel
			UINT uMIDIChannel = (UINT)vstEvent.polyPressure.channel;
			UINT uMIDINote = (UINT)vstEvent.polyPressure.pitch;
			float fPressure = vstEvent.polyPressure.pressure;

			// --- test channel/ignore
			if(m_uMidiRxChannel != MIDI_CH_ALL && uMIDIChannel != m_uMidiRxChannel)
				return false;

			// --- note event did not occurr
			noteEvent = false;

			#if(LOG_MIDI && _DEBUG)
				FDebugPrint("-- Poly Pressure Ch:%d Note:%d Value:%d \n", uMIDIChannel, uMIDINote, fPressure);
			#endif

			break;
		}
	}

	// -- note event occurred?
	return noteEvent;
}

/*
	Processor::process()
	The most important function handles:
		Control Changes (same as userInterfaceChange() in RAFX)
		Synth voice rendering
		Output GUI Changes (allows you to write back to the GUI, advanced.
*/
tresult PLUGIN_API Processor::process(ProcessData& data)
{
	// --- check for presence of output buffers
	if (!data.outputs)
		return kResultTrue;

	// --- check for conrol chages and update synth if needed
	doControlUpdate(data);

	// --- sticky ON
	tresult result = kResultFalse;

	// --- we process 32 samples at a time; MIDI events are then accurate to 0.7 mSec
	const int32 kBlockSize = SYNTH_PROC_BLOCKSIZE;

	// 32-bit is float
	// if doing a 64-bit version, you need to replace with double*
	// initialize audio output buffers
	float* buffers[OUTPUT_CHANNELS]; // Precision is float - need to change this do DOUBLE if supporting 64 bit

	// 32-bit is float
	// if doing a 64-bit version, you need to replace with double* here too
	for(int i = 0; i < OUTPUT_CHANNELS; i++)
	{
		// data.outputs[0] = BUS 0
		buffers[i] = (float*)data.outputs[0].channelBuffers32[i];
		memset (buffers[i], 0, data.numSamples * sizeof(float));
	}

	// --- total number of samples in the input Buffer
	int32 numSamples = data.numSamples;

	// --- this is used when we need to shove an event into the next block
	int32 samplesProcessed = 0;

	// --- get our list of events
	IEventList* inputEvents = data.inputEvents;
	Event e = {0};
	Event* eventPtr = 0;
	int32 eventIndex = 0;

	// --- count of events
	int32 numEvents = inputEvents ? inputEvents->getEventCount () : 0;

	// get the first event
	if(numEvents)
	{
		inputEvents->getEvent (0, e);
		eventPtr = &e;
	}

	while(numSamples > 0)
	{
		// bound the samples to process to BLOCK SIZE (32)
		int32 samplesToProcess = std::min<int32> (kBlockSize, numSamples);

		while(eventPtr != 0)
		{
			// if the event is not in the current processing block then adapt offset for next block
			if (e.sampleOffset > samplesToProcess)
			{
				e.sampleOffset -= samplesToProcess;
				break;
			}

			// --- find MIDI note-on/off and broadcast
			doProcessEvent(e);

			// --- get next event
			eventIndex++;
			if (eventIndex < numEvents)
			{
				if (inputEvents->getEvent (eventIndex, e) == kResultTrue)
				{
					e.sampleOffset -= samplesProcessed;
				}
				else
				{
					eventPtr = 0;
				}
			}
			else
			{
				eventPtr = 0;
			}
		}

		// the loop - samplesToProcess is more like framesToProcess
		// sine we will to a pair of samples at a time
		float fMix = 0.25; // 12dB headroom
		double dLeft = 0.0;
		double dRight = 0.0;
		double dLeftAccum = 0.0;
		double dRightAccum = 0.0;

		for(int32 j=0; j<samplesToProcess; j++)
		{
			// --- clear accumulators
			dLeftAccum = 0.0;
			dRightAccum = 0.0;

			for(int i=0; i<MAX_VOICES; i++)
			{
				// --- iterate and process; notice we process ALL voices
				//     if they are OFF they return 0.0 immediately
				// --- render left and right
				if(m_pVoiceArray[i])
					m_pVoiceArray[i]->doVoice(dLeft, dRight);

				// --- accumulate notes
				dLeftAccum += fMix*dLeft;
				dRightAccum += fMix*dRight;
			}

			// write out to buffer
			buffers[0][j] = dLeftAccum;  // left
			buffers[1][j] = dRightAccum; // right
		}

		// --- update the counter
		for(int i = 0; i < OUTPUT_CHANNELS; i++)
			buffers[i] += samplesToProcess;

		// --- update the samples processed/to process
		numSamples -= samplesToProcess;
		samplesProcessed += samplesToProcess;

	} // end while (numSamples > 0)

	// can write OUT to the GUI; see documentation
	if (data.outputParameterChanges)
	{
		// write to GUI or Parameters
	}

	// --- set silence flags if no notes playing
	if(getNoteOnCount() == 0 && data.numOutputs > 0)
	{
		data.outputs[0].silenceFlags = 0x11; // left and right channel are silent
	}

	return kResultTrue;
}

}}} // namespaces
