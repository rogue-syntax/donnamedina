/*
 *  mdaBaseProcessor.cpp
 *  mda-vst3
 *
 *  Created by Arne Scheffler on 6/14/08.
 *
 *  mda VST Plug-ins
 *
 *  Copyright (c) 2008 Paul Kellett
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "mdaBaseProcessor.h"
#include "mdaBaseController.h"
#include "public.sdk/source/common/memorystream.h"
#include "helpers.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/base/ibstream.h"

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <xmmintrin.h>  // _mm_getcsr / _mm_setcsr (FTZ/DAZ)
#include "base/source/fstreamer.h"



namespace Steinberg {
namespace Vst {
namespace rs {

namespace {
// B-10: true while this thread is inside BaseProcessor::process() (the host's audio thread).
thread_local bool tl_inProcess = false;
}

//-----------------------------------------------------------------------------
BaseProcessor::BaseProcessor ()
: params (0)
, numParams (0)
, bypassRamp (0)
, bypassBuffer0 (0)
, bypassBuffer1 (0)
, bypassState (false)
, msgQueue (kMsgQueueSize)
{
	for (int k = 0; k < kPlayheadSlots; k++) {
		phPitch[k].store (-1);
		phAge[k].store (0.0f);
		phRel[k].store (-1.0f);
		phLevel[k].store (0.0f);
		for (int j = 0; j < 6; j++)
			phSrc[k][j].store (0.0f);
	}
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseProcessor::initialize (FUnknown* context)
{
	tresult res = AudioEffect::initialize (context);
	// Called on the host's main thread; the SDK timer fires on this thread (B-10).
	if (res == kResultTrue && !msgTimer)
		msgTimer = Timer::create (this, kMsgTimerMs);
	return res;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseProcessor::terminate ()
{
	if (msgTimer) {
		msgTimer->stop ();
		msgTimer->release ();
		msgTimer = nullptr;
	}
	return AudioEffect::terminate ();
}

//-----------------------------------------------------------------------------
bool BaseProcessor::onAudioThread () { return tl_inProcess; }

void BaseProcessor::onTimer (Timer* /*timer*/)
{
	flushMessageQueue ();
	sendScopeFrame ();
	sendVoiceStates ();
	sendPlayheads ();
	sendPeak ();
}

void BaseProcessor::notePeak (float blockPeak)
{
	float cur = outPeak.load (std::memory_order_relaxed);
	while (blockPeak > cur && !outPeak.compare_exchange_weak (cur, blockPeak, std::memory_order_relaxed)) {
	}
}

void BaseProcessor::sendPeak ()
{
	if (!scopeEnabled.load (std::memory_order_relaxed))
		return;
	float peak = outPeak.exchange (0.0f, std::memory_order_relaxed);
	if (!(peak == peak)) peak = 1.0e9f;  // NaN in the output: show it as clipping
	const bool zero = peak < 1.0e-6f;
	if (zero && peakLastSentZero)
		return;
	peakLastSentZero = zero;
	IMessage* msg = allocateMessage ();
	if (!msg)
		return;
	msg->setMessageID (msgIdStr (PMSG_PEAK_CHAR));
	msg->getAttributes ()->setFloat ("peak", zero ? 0.0 : double (peak));
	msg->getAttributes ()->setInt ("id", this->myInstanceId);
	sendMessage (msg);
	msg->release ();
}

void BaseProcessor::updateVoiceStates ()
{
	// One voice per pitch (RVoicesMap[pitch]); `notes` lists the ones being rendered.
	const DXGenerator& g = generators[0];
	uint8 now[128] = {};
	if (g.RVoicesMap) {
		for (int i = 0; i < g.notes.size (); i++) {
			const int32 pitch = g.notes[i];
			if (pitch < 0 || pitch >= 128)
				continue;
			const RVoice& v = g.RVoicesMap[pitch];
			now[pitch] = voiceStateByte (v.noteActive, v.voiceEnvLfo[1].cenv);
		}
	}
	for (int p = 0; p < 128; p++)
		voiceStates[p].store (now[p], std::memory_order_relaxed);

	// Playheads: the most recently (re)started voices, newest first. Voices being stolen (killIt)
	// are skipped: they are fading out for a new note.
	int best[kPlayheadSlots];
	unsigned int bestSerial[kPlayheadSlots];
	int nBest = 0;
	if (g.RVoicesMap) {
		for (int i = 0; i < g.notes.size (); i++) {
			const int32 pitch = g.notes[i];
			if (pitch < 0 || pitch >= 128)
				continue;
			const RVoice& v = g.RVoicesMap[pitch];
			if (v.killIt)
				continue;
			int at = nBest;
			while (at > 0 && bestSerial[at - 1] < v.serial)
				at--;
			if (at >= kPlayheadSlots)
				continue;
			const int last = nBest < kPlayheadSlots ? nBest : kPlayheadSlots - 1;
			for (int k = last; k > at; k--) {
				best[k] = best[k - 1];
				bestSerial[k] = bestSerial[k - 1];
			}
			best[at] = pitch;
			bestSerial[at] = v.serial;
			if (nBest < kPlayheadSlots)
				nBest++;
		}
	}
	const double sr = g.sampleRate > 0.0f ? g.sampleRate : 44100.0;
	for (int k = 0; k < kPlayheadSlots; k++) {
		if (k >= nBest) {
			phPitch[k].store (-1, std::memory_order_relaxed);
			continue;
		}
		const RVoice& v = g.RVoicesMap[best[k]];
		const float age = static_cast<float> (double (g.sampleClock - v.startClock) / sr);
		float rel = -1.0f;
		if (!v.noteActive && v.releaseClock >= v.startClock)
			rel = static_cast<float> (double (g.sampleClock - v.releaseClock) / sr);
		phAge[k].store (age, std::memory_order_relaxed);
		phRel[k].store (rel, std::memory_order_relaxed);
		phLevel[k].store (v.voiceEnvLfo[1].cenv, std::memory_order_relaxed);
		for (int j = 0; j < 6; j++)
			phSrc[k][j].store (v.voiceEnvLfo[j + 1].cenv, std::memory_order_relaxed);
		phPitch[k].store (best[k], std::memory_order_relaxed);
	}
}

void BaseProcessor::clearPlayheads ()
{
	for (int k = 0; k < kPlayheadSlots; k++)
		phPitch[k].store (-1, std::memory_order_relaxed);
}

void BaseProcessor::sendPlayheads ()
{
	if (!scopeEnabled.load (std::memory_order_relaxed))
		return;
	PlayheadSlot s[kPlayheadSlots];
	bool empty = true;
	for (int k = 0; k < kPlayheadSlots; k++) {
		s[k].pitch = phPitch[k].load (std::memory_order_relaxed);
		s[k].age = phAge[k].load (std::memory_order_relaxed);
		s[k].relAge = phRel[k].load (std::memory_order_relaxed);
		s[k].level = phLevel[k].load (std::memory_order_relaxed);
		for (int j = 0; j < 6; j++)
			s[k].src[j] = phSrc[k][j].load (std::memory_order_relaxed);
		if (s[k].pitch >= 0)
			empty = false;
	}
	if (empty && playheadsLastSentEmpty)
		return;
	playheadsLastSentEmpty = empty;
	IMessage* msg = allocateMessage ();
	if (!msg)
		return;
	msg->setMessageID (msgIdStr (PMSG_PLAYHEADS_CHAR));
	msg->getAttributes ()->setBinary ("data", s, sizeof (s));
	msg->getAttributes ()->setInt ("id", this->myInstanceId);
	sendMessage (msg);
	msg->release ();
}

void BaseProcessor::sendVoiceStates ()
{
	if (!scopeEnabled.load (std::memory_order_relaxed))
		return;
	uint8 now[128];
	bool changed = !voiceStatesSentValid;
	for (int p = 0; p < 128; p++) {
		now[p] = voiceStates[p].load (std::memory_order_relaxed);
		changed = changed || now[p] != voiceStatesSent[p];
	}
	if (!changed)
		return;
	IMessage* msg = allocateMessage ();
	if (!msg)
		return;
	msg->setMessageID (msgIdStr (PMSG_VOICES_CHAR));
	msg->getAttributes ()->setBinary ("data", now, sizeof (now));
	msg->getAttributes ()->setInt ("id", this->myInstanceId);
	sendMessage (msg);
	msg->release ();
	std::memcpy (voiceStatesSent, now, sizeof (now));
	voiceStatesSentValid = true;
}

void BaseProcessor::sendScopeFrame ()
{
	if (!scopeEnabled.load (std::memory_order_relaxed))
		return;
	const float* frame = scope.latest ();
	if (!frame)
		return;
	float peak = 0.0f;
	for (int i = 0; i < ScopeCapture::kLength; i++)
		peak = (std::max) (peak, std::fabs (frame[i]));
	const bool silent = peak < 1.0e-5f;
	if (silent && scopeLastSentSilent)
		return;  // the display already shows a flat line
	scopeLastSentSilent = silent;
	IMessage* msg = allocateMessage ();
	if (!msg)
		return;
	msg->setMessageID (msgIdStr (PMSG_SCOPE_CHAR));
	msg->getAttributes ()->setBinary ("data", frame, sizeof (float) * ScopeCapture::kLength);
	msg->getAttributes ()->setInt ("id", this->myInstanceId);
	sendMessage (msg);
	msg->release ();
}

void BaseProcessor::flushMessageQueue ()
{
	QueuedMsg m;
	while (msgQueue.pop (m))
		sendNowOrQueue (m, true);
}

void BaseProcessor::sendNowOrQueue (const QueuedMsg& m, bool fromQueue)
{
	if (!fromQueue && onAudioThread ()) {
		if (!msgQueue.push (m))
			msgQueueDropped++;
		return;
	}
	IMessage* msg = allocateMessage ();
	if (!msg)
		return;
	// The ID is sent as a proper 1-char C string. The controller only reads the first char; the
	// old `&PMSG_..._CHAR` (address of a single char) was not NUL-terminated, so setMessageID read
	// past it (B-53).
	const char id[2] = {m.id, 0};
	msg->setMessageID (id);
	switch (m.kind) {
		case QueuedMsg::kDoubleAttr:
			msg->getAttributes ()->setFloat (m.attr, m.a);
			break;
		case QueuedMsg::kPidVal:
			msg->getAttributes ()->setInt ("pid", static_cast<int64> (m.a));
			msg->getAttributes ()->setFloat ("val", m.b);
			break;
		case QueuedMsg::kIntAttr:
			msg->getAttributes ()->setInt (m.attr, static_cast<int64> (m.a));
			break;
	}
	msg->getAttributes ()->setInt ("id", this->myInstanceId);
	sendMessage (msg);
	msg->release ();
}

//-----------------------------------------------------------------------------
BaseProcessor::~BaseProcessor ()
{
	if (bypassBuffer0)
		std::free (bypassBuffer0);
	if (bypassBuffer1)
		std::free (bypassBuffer1);
	if (params)
		delete [] params;
}

//-----------------------------------------------------------------------------

tresult PLUGIN_API BaseProcessor::process (ProcessData& data)
{
	// Optimization / B-40: flush denormals to zero while we render (FTZ + DAZ). Decaying filter and
	// envelope tails otherwise pass through denormal range, which is many times slower. The host's
	// flags are restored before returning.
	const unsigned int savedCsr = _mm_getcsr ();
	_mm_setcsr (savedCsr | 0x8040);  // bit 15 FTZ, bit 6 DAZ

	tl_inProcess = true;  // B-10: messages sent from here on are queued, not sent
	preProcess(data);
	tl_inProcess = false;

	_mm_setcsr (savedCsr);
	/*
	if (processParameterChanges(data.inputParameterChanges))
		
		recalculate ();

		processEvents (data.inputEvents);

	if (data.numSamples > 0 && !bypassProcessing (data)) 
	{
		doProcessing (data);
		checkSilence (data);
	}
		*/
	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseProcessor::setupProcessing (ProcessSetup& setup)
{
	if (bypassBuffer0)
		std::free (bypassBuffer0);
	bypassBuffer0 = (float*)std::malloc (setup.maxSamplesPerBlock * sizeof (float));

	if (bypassBuffer1)
		std::free (bypassBuffer1);
	bypassBuffer1 = (float*)std::malloc (setup.maxSamplesPerBlock * sizeof (float));
	
	return AudioEffect::setupProcessing (setup);
}

//-----------------------------------------------------------------------------
bool BaseProcessor::bypassProcessing (ProcessData& data)
{
	if (data.numSamples == 0)
		return true;

	if (isBypassed () || bypassRamp != 0)
	{
		if (bypassRamp)
		{
			float* origOut0 = data.outputs[0].channelBuffers32[0];
			float* origOut1 = data.outputs[0].channelBuffers32[1];

			data.outputs[0].channelBuffers32[0] = bypassBuffer0;
			data.outputs[0].channelBuffers32[1] = bypassBuffer1;

			doProcessing (data);

			data.outputs[0].channelBuffers32[0] = origOut0;
			data.outputs[0].channelBuffers32[1] = origOut1;

			float* bpb0 = bypassBuffer0;
			float* bpb1 = bypassBuffer1;
			float origMix = 0;
			float plugMix = 0;
			float factor = 1.f / (float)data.numSamples;

			if (bypassRamp > 0)
			{
				for (int32 i = 0; i < data.numSamples; i++)
				{
					origMix = (float)i * factor;
					plugMix = (float)(data.numSamples - i) * factor;
					*origOut0 = *origOut0 * origMix + *bpb0 * plugMix;
					*origOut1 = *origOut1 * origMix + *bpb1 * plugMix;
					origOut0++;
					origOut1++;
					bpb0++;
					bpb1++;
				}
			}
			else
			{
				for (int32 i = 0; i < data.numSamples; i++)
				{
					plugMix = (float)i * factor;
					origMix = (float)(data.numSamples - i) * factor;
					*origOut0 = *origOut0 * origMix + *bpb0 * plugMix;
					*origOut1 = *origOut1 * origMix + *bpb1 * plugMix;
					origOut0++;
					origOut1++;
					bpb0++;
					bpb1++;
				}
			}
			data.outputs[0].silenceFlags = 0;
			bypassRamp = 0;
		}
		else
		{
			for (int32 bus = 0; bus < data.numInputs, bus < data.numOutputs; bus++)
			{
				Bus* output = getAudioOutput (bus);
				if (!output || !output->isActive ())
					continue;
				for (int32 channel = 0; channel < data.outputs[bus].numChannels; channel++)
				{
					if (data.numInputs <= bus || data.inputs[bus].numChannels <= channel)
					{
						memset (data.outputs[bus].channelBuffers32[channel], 0, sizeof (float) * data.numSamples);
						data.outputs[bus].silenceFlags |= (uint64)1 << channel;
					}
					else
					{
						if (data.outputs[bus].channelBuffers32[channel] != data.inputs[bus].channelBuffers32[channel])
							memcpy (data.outputs[bus].channelBuffers32[channel], data.inputs[bus].channelBuffers32[channel], sizeof (float) * data.numSamples);
 						data.outputs[bus].silenceFlags |= data.inputs[bus].silenceFlags & ((uint64)1 << channel);
					}
				}
			}
		}
		return true;
	}
	return false;
}

#define kSilenceThreshold 0.000132184039

//-----------------------------------------------------------------------------
void BaseProcessor::checkSilence (ProcessData& data)
{
	for (int32 bus = 0; bus < data.numOutputs; bus ++)
	{
		data.outputs[bus].silenceFlags = 0;
		
		if (!getAudioOutput (bus)->isActive ())
			continue;

		for (int32 channel = 0; channel < data.outputs[bus].numChannels; channel++)
		{
			bool channelIsSilent = true;
			for (int32 sample = 0; sample < data.numSamples; sample += 10)
			{
				if (fabs (data.outputs[bus].channelBuffers32[channel][sample]) > kSilenceThreshold)
				{
					channelIsSilent = false;
					break;
				}
			}
			if (channelIsSilent)
				data.outputs[bus].silenceFlags |= (uint64)1 << channel;
		}
	}
}

//-----------------------------------------------------------------------------
bool BaseProcessor::processParameterChanges (IParameterChanges* changes){
	int a = 0;
	int32 count = changes->getParameterCount ();
	if (count > 0){
		for (int32 i = 0; i < count; i++)
		{
			IParamValueQueue* queue = changes->getParameterData (i);
			if (!queue)
				continue;
			ParamID paramId = queue->getParameterId ();
			int32 pointCount = queue->getPointCount ();
			int32 sampleOffset;
			ParamValue value;
			queue->getPoint (pointCount - 1, sampleOffset, value);
			if (paramId == BaseController::kBypassParam)
				setBypass (value >= 0.5, sampleOffset);
			else if (paramId == BaseController::kPresetParam)
				setCurrentProgramNormalized (value);
			else
				setParameter (paramId, value, sampleOffset);
		}
		return true;
	}else {
		return false;
	}
	
	
}

//-----------------------------------------------------------------------------
void BaseProcessor::setBypass (bool state, int32 sampleOffset)
{
	if (state != bypassState)
	{
		bypassState = state;
		if (bypassState)
			bypassRamp = 1;
		else
			bypassRamp = -1;
	}
}

//-----------------------------------------------------------------------------
void BaseProcessor::setParameter (ParamID index, ParamValue newValue, int32 sampleOffset){
	if (index < GPARAMS) {
		floatParams[index] = newValue;
	}

}

//-----------------------------------------------------------------------------
void BaseProcessor::allocParameters (int32 _numParams)
{
	if (params) {
		return;
	}
		
	numParams = _numParams;
	//floatParams = new ParamValue[numParams];
	//old
	params = new ParamValue[numParams];

	
	
	//for (int32 i = 0; i < GENCOUNT; i++) {
	//	generators[i].partParams = new ParamValue[numParams];
		
	//}


}

//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseProcessor::setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
	if (numIns)
	{
		if (SpeakerArr::getChannelCount (inputs[0]) != 2)
			return kResultFalse;
	}
	if (numOuts)
	{
		if (SpeakerArr::getChannelCount (outputs[0]) != 2)
			return kResultFalse;
	}
	return kResultTrue;
}


tresult PLUGIN_API BaseProcessor::notify(IMessage* message) {

	
	/*
	if (strcmp(message->getMessageID(), "paramKeys") == 0) {
		int64 paramID;
		String128 name;
		message->getAttributes()->getInt("id", paramID);
		message->getAttributes()->getString("name", name, 128 * sizeof(TChar));
		uint32 pid = paramID;
		UString128 tmp(name);
		char ascii[128];
		tmp.toAscii(ascii, 128);
		//string transmission doesnt work for me
		return kResultTrue;
	}
	*/
	if (message && message->getMessageID () && message->getMessageID ()[0] == CMSG_SCOPE_ENABLE_CHAR) {
		int64 on = 0;
		message->getAttributes ()->getInt ("on", on);
		scopeEnabled.store (on != 0);
		scopeLastSentSilent = false;  // the editor starts empty: send the next frame even if flat
		voiceStatesSentValid = false;  // and the voice strip in full
		playheadsLastSentEmpty = false;
		peakLastSentZero = false;
		outPeak.store (0.0f, std::memory_order_relaxed);
		if (on == 0) {
			for (int p = 0; p < 128; p++) voiceStates[p].store (0, std::memory_order_relaxed);
			clearPlayheads ();
		}
		return kResultTrue;
	}
	// Preset bar: load a state the controller built (preset file, factory preset, randomized), or
	// send the current one back (save, randomize). notify runs on the main thread, like a host's
	// own setState/getState.
	if (message && message->getMessageID () && message->getMessageID ()[0] == CMSG_SET_STATE_CHAR
		&& message->getMessageID ()[1] == 0) {
		const void* data = nullptr;
		uint32 size = 0;
		if (message->getAttributes ()->getBinary ("data", data, size) == kResultTrue && data && size > 0) {
			MemoryStream ms (const_cast<void*> (data), static_cast<TSize> (size));
			setState (&ms);
		}
		return kResultTrue;
	}
	if (message && message->getMessageID () && message->getMessageID ()[0] == CMSG_GET_STATE_CHAR
		&& message->getMessageID ()[1] == 0) {
		int64 purpose = 0;
		message->getAttributes ()->getInt ("purpose", purpose);
		MemoryStream ms;
		if (getState (&ms) == kResultTrue) {
			if (IMessage* reply = allocateMessage ()) {
				reply->setMessageID (msgIdStr (PMSG_STATE_CHAR));
				reply->getAttributes ()->setBinary ("data", ms.getData (), static_cast<uint32> (ms.getSize ()));
				reply->getAttributes ()->setInt ("purpose", purpose);
				sendMessage (reply);
				reply->release ();
			}
		}
		return kResultTrue;
	}
	return ComponentBase::notify(message);
}


void BaseProcessor::addParameter(char* titler, char* units,
	int32 stepCount, ParamValue defaultNormalizedValue,
	int32 flags, int32 tag) {
	char* title = titler;
	paramKeys[title] = tag;

}



//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseProcessor::setActive (TBool state)
{
	/*
	if (state){
		IMessage* msg = allocateMessage ();
		if (msg)
		{
			msg->setMessageID (0);
			msg->getAttributes ()->setFloat ("SampleRate", processSetup.sampleRate);
			sendMessage (msg);
			msg->release ();
		}
	}
	*/
	for (genRecalcIdx = 0; genRecalcIdx < GENCOUNT; genRecalcIdx++) {
		generators[genRecalcIdx].sampleRateDouble = getSampleRate();
		generators[genRecalcIdx].sampleRate = (float)generators[genRecalcIdx].sampleRateDouble;
		generators[genRecalcIdx].ifs = 1.0f / generators[genRecalcIdx].sampleRateDouble;
		generators[genRecalcIdx].recalculate(generators[genRecalcIdx].sampleRate);//huh
		generators[genRecalcIdx].createVoice();
	}
	if (state)
		scope.setSampleRate (getSampleRate ());

	return AudioEffect::setActive (state);
}

//tresult PLUGIN_API BaseProcessor::sendDoubleAttr(const tresult* msgID, const char* attrID, const double& value){
// msgID is the address of a single-char ID constant (e.g. &PMSG_LFO4_RATE_CHAR); only its first
// char is used. attrID must be a string literal (it is stored until the queue is flushed).
tresult PLUGIN_API BaseProcessor::sendDoubleAttr(const char* msgID, const char* attrID, const double& value) {
	QueuedMsg m;
	m.kind = QueuedMsg::kDoubleAttr;
	m.id = msgID ? msgID[0] : 0;
	m.attr = attrID;
	m.a = value;
	sendNowOrQueue(m);
	return kResultTrue;
}

tresult PLUGIN_API BaseProcessor::sendPidVal(const char* msgID, const double& pid, const double& val) {
	QueuedMsg m;
	m.kind = QueuedMsg::kPidVal;
	m.id = msgID ? msgID[0] : 0;
	m.a = pid;
	m.b = val;
	sendNowOrQueue(m);
	return kResultTrue;
}

tresult PLUGIN_API BaseProcessor::sendIntAttr(const char* msgID, const char* attrID, const int& value) {
	QueuedMsg m;
	m.kind = QueuedMsg::kIntAttr;
	m.id = msgID ? msgID[0] : 0;
	m.attr = attrID;
	m.a = value;
	sendNowOrQueue(m);
	return kResultTrue;
}



void BaseProcessor::recalculate() {

	for (genRecalcIdx = 0; genRecalcIdx < GENCOUNT; genRecalcIdx++) {
		generators[genRecalcIdx].sampleRateDouble = getSampleRate();
		generators[genRecalcIdx].sampleRate = (float)generators[genRecalcIdx].sampleRateDouble;
		generators[genRecalcIdx].ifs = 1.0f / generators[genRecalcIdx].sampleRateDouble;
		//generators[genRecalcIdx].recalculate(generators[genRecalcIdx].sampleRate);
	}


}


//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseProcessor::setState (IBStream* state){
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	// B-08: robust against short, longer or damaged states:
	// - the old loop `i < temp, i < GPARAMS` used the comma operator, so it always read GPARAMS
	//   values whatever the stream held; on a shorter state it read past the data and then took
	//   the stale parameter count as the bypass flag (bypass on = silence).
	// - read results were not checked; the program number was not range-checked.
	uint32 temp = 0;
	if (!streamer.readInt32u (temp)) // numParams or Header
		return kResultFalse;

	if (temp == BaseController::kMagicNumber)
	{
		// read current Program (setCurrentProgram clamps it)
		uint32 program = 0;
		if (!streamer.readInt32u (program))
			return kResultFalse;
		setCurrentProgram (program);

		if (!streamer.readInt32u (temp))
			return kResultFalse;
	}

	// read each parameter: as many as the state has, at most GPARAMS; skip any extra ones
	const uint32 count = temp;
	for (uint32 i = 0; i < count; i++)
	{
		double v = 0.0;
		if (!streamer.readDouble (v))
			return kResultFalse;  // truncated: keep what was read so far
		if (i < GPARAMS && std::isfinite (v))
			floatParams[i] = v;
	}

	// bypass (older states may end before it)
	uint32 bypass = 0;
	if (streamer.readInt32u (bypass))
		bypassState = bypass > 0;

	// master volume: a tagged block after bypass (written by getState since 2026-11-14). States
	// without it load at 0 dB; the array slot is ignored because older states hold 0 there.
	floatParams[MASTER_VOL] = DEFAULT_MASTER_VOL;
	uint32 tag = 0;
	double masterVol = 0.0;
	if (streamer.readInt32u (tag) && tag == MASTER_VOL_STATE_TAG && streamer.readDouble (masterVol)
		&& std::isfinite (masterVol))
		floatParams[MASTER_VOL] = (std::min) (1.0, (std::max) (0.0, masterVol));

	// polyphony: the next tagged block (2026-11-15); without it the default
	floatParams[POLY_COUNT] = polyToNormalized (POLY_DEFAULT);
	double poly = 0.0;
	if (streamer.readInt32u (tag) && tag == POLY_STATE_TAG && streamer.readDouble (poly) && std::isfinite (poly))
		floatParams[POLY_COUNT] = (std::min) (1.0, (std::max) (0.0, poly));

	//recalculate ();

	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseProcessor::getState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	if (hasProgram ())
	{
		// save header key
		uint32 temp = BaseController::kMagicNumber;
		streamer.writeInt32u (temp);

		// save program
		temp = getCurrentProgram ();
		streamer.writeInt32u (temp);
	}

	// save number of parameter (B-08: the count of values written below; numParams is GPARAMS too,
	// so saved states are byte-identical to before)
	streamer.writeInt32u (static_cast<uint32> (GPARAMS));

	// save each parameter
	for (uint32 i = 0; i < GPARAMS; i++)
	{
		streamer.writeDouble (floatParams[i]);
	}

	// save bypass
	streamer.writeInt32u (bypassState ? 1 : 0);

	// master volume (see setState); older builds stop reading after bypass and ignore this
	streamer.writeInt32u (MASTER_VOL_STATE_TAG);
	streamer.writeDouble (floatParams[MASTER_VOL]);
	streamer.writeInt32u (POLY_STATE_TAG);
	streamer.writeDouble (floatParams[POLY_COUNT]);

	return kResultTrue;
}
}}} // namespace
