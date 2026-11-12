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
#include "helpers.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/base/ibstream.h"

#include <cmath>
#include <cstdlib>
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

void BaseProcessor::onTimer (Timer* /*timer*/) { flushMessageQueue (); }

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
	tl_inProcess = true;  // B-10: messages sent from here on are queued, not sent
	preProcess(data);
	tl_inProcess = false;
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

	uint32 temp;
	streamer.readInt32u (temp); // numParams or Header

	if (temp == BaseController::kMagicNumber)
	{
		// read current Program
		streamer.readInt32u (temp);
		setCurrentProgram (temp);

		streamer.readInt32u (temp);
	}

	// read each parameter
	for (uint32 i = 0; i < temp, i < GPARAMS; i++)
	{
		//streamer.readDouble (params[i]);
		streamer.readDouble(floatParams[i]);
	}

	// bypass
	streamer.readInt32u (temp);
	bypassState = temp > 0;

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

	// save number of parameter
	streamer.writeInt32u (numParams);

	// save each parameter
	for (uint32 i = 0; i < GPARAMS; i++)
	{
		streamer.writeDouble (floatParams[i]);
	}

	// save bypass
	streamer.writeInt32u (bypassState ? 1 : 0);

	return kResultTrue;
}
}}} // namespace
