//-----------------------------------------------------------------------------
// Project     : DX SYNTH
// Version     : 3.6.6
// Created by  : Will Pirkle
// Description : FM Synth
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2013, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "VSTSynthController.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/futils.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "base/source/fstring.h"
#include "logscale.h"
#include "base/source/fstreamer.h"
#include "synthfunctions.h"
#include "synthparamlimits.h" // param limits file

namespace Steinberg {
namespace Vst {
namespace rs {

// --- the unique identifier (use guidgen.exe to generate)
FUID Controller::cid (0x0D5BA01E, 0x428A41C4, 0xACB7067B, 0x6EA1085E);



// this defines a logarithmig scaling for the filter Fc control
//LogScale<ParamValue> filterLogScale (0.0,		/* VST GUI Variable MIN */
//									 1.0,		/* VST GUI Variable MAX */
//									 80.0,		/* filter fc LOW */
//									 18000.0,	/* filter fc HIGH */
//									 0.5,		/* the value at position 0.5 will be: */
//									 1800.0);	/* 1800 Hz */

/* 
	Controller::initialize()

	This is where we describe the Default GUI controls and link them to control IDs.
	This must be done if you want a Custom GUI too so you have to implement this no matter what.
*/
tresult PLUGIN_API Controller::initialize(FUnknown* context)
{
	// --- base class does its thing
	tresult result = EditController::initialize(context);

	// --- now define the controls
	if (result == kResultTrue)
	{
	// Init parameters
		Parameter* param;


		StringListParameter* enumStringParam = new StringListParameter (USTRING("Algorithm"), VOICE_MODE);
		// now add the strings for the list IN THE SAME ORDER AS DECLARED IN THE enum in Synth Project
		enumStringParam->appendString(USTRING("DX1"));		// <- 0
		enumStringParam->appendString(USTRING("DX2"));		// <- 1
		enumStringParam->appendString(USTRING("DX3"));		// <- 1
		enumStringParam->appendString(USTRING("DX4"));		// <- 1
		enumStringParam->appendString(USTRING("DX5"));		// <- 1
		enumStringParam->appendString(USTRING("DX6"));		// <- 1
		enumStringParam->appendString(USTRING("DX7"));		// <- 1
		enumStringParam->appendString(USTRING("DX8"));		// <- 1
		parameters.addParameter (enumStringParam);

		param = new RangeParameter(USTRING("Volume"), OUTPUT_AMPLITUDE_DB, USTRING("dB"), MIN_OUTPUT_AMPLITUDE_DB, MAX_OUTPUT_AMPLITUDE_DB, DEFAULT_OUTPUT_AMPLITUDE_DB);
		param->setPrecision(1); // fractional sig digits
		parameters.addParameter(param);

		
	}

	return kResultTrue;
}

/* 
	Controller::terminate()
	The End.
*/
tresult PLUGIN_API Controller::terminate()
{
	return EditController::terminate();
}

/* 
	Controller::setParamNormalizedFromFile()
	This sets the normalized (raw) parameter value
	from the file's cooked (plain) value

	tag - the Index value of a control
	value - the cooked value from the file
*/
tresult PLUGIN_API Controller::setParamNormalizedFromFile(ParamID tag, ParamValue value)
{
	// --- get the parameter
	//Parameter* pParam = EditController::getParameterObject(tag);
	Parameter* pParam = getParameterObject(tag);

	// --- verify pointer
	if(!pParam) return kResultFalse; 

	//  --- convert serialized value to normalized (raw)
	return setParamNormalized(tag, pParam->toNormalized(value));
}

// NEED TO IMPLEMENMT THISSS! For restoring the UI state
// it is already done in the processor correctly but needs to be set here too
tresult PLUGIN_API Controller::setComponentState(IBStream* fileStream)
{
	IBStreamer s(fileStream, kLittleEndian);
	uint64 version = 0;
	double dDoubleParam = 0;

	// --- needed to convert to our UINT reads
	uint32 udata = 0;
	int32 data = 0;

	// read the version
	if(!s.readInt64u(version)) return kResultFalse;

	// --- then read Version 0 params
	if(!s.readInt32u(udata)) 
		return kResultFalse; 
	else 
		setParamNormalizedFromFile(VOICE_MODE, (ParamValue)udata);

	if(!s.readDouble(dDoubleParam)) 
		return kResultFalse;
	else
		setParamNormalizedFromFile(OUTPUT_AMPLITUDE_DB, dDoubleParam);

	// --- do next version...
	if (version >= 1)
	{
		// add v1 stuff here
	}


	return kResultTrue;
}

/* 
	Controller::getParamNormalized()
	Called by client to get the noramalized parameter for drawing the GUI.
*/
ParamValue PLUGIN_API Controller::getParamNormalized(ParamID tag)
{
	// ordinarily the base class can handle this on its own, since we set the min,max,default 
	// when we constructed the params. But if you need to massage the value any more, here
	// is where you can do it.
	ParamValue paramValue = EditController::getParamNormalized(tag); 

	//switch(tag) // same as RAFX uControlID
	//{
	//	case VOICE_MODE:
	//	{
	//		// get the parameter, then do something with it...
	//		Parameter* pParam = EditController::getParameterObject(tag);

	//		ParamValue = ...
	//		break;
	//	}

	return paramValue;
}

/* 
	Controller::setParamNormalized()
	Called by GUI when a control is changed to update itself.
*/
tresult PLUGIN_API Controller::setParamNormalized (ParamID tag, ParamValue value)
{
	// --- this is called to update the GUI, eg when you turn a knob, this would
	//     update the edit control with the new value

	return EditController::setParamNormalized(tag, value);
	//return setParamNormalized(tag, value);
}

/* --- IMIDIMapping Interface
	Controller::getMidiControllerAssignment()

	The client queries this 129 times for 130 possible control messages, see ivstsmidicontrollers.h for 
	the VST defines for kPitchBend, kCtrlModWheel, etc... for each MIDI Channel in our Event Bus

	We respond with our ControlID value that we'll use to process the MIDI Messages in Processor::process().

	On the default GUI, these controls will actually move with the MIDI messages, but we don't want that on 
	the final UI so that we can have any Modulation Matrix mapping we want.
*/
tresult PLUGIN_API Controller::getMidiControllerAssignment(int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& id/*out*/)
{
	// NOTE: we only have one EventBus(0)
	//       but it has 16 channels on it
	if(busIndex == 0)
	{
		id = -1;
		switch (midiControllerNumber)
		{
			// these messages handled in the Processor::process() method
			case kPitchBend: 
				id = MIDI_PITCHBEND; 
				break;
			case kCtrlModWheel: 
				id = MIDI_MODWHEEL; 
				break;
			case kCtrlVolume: 
				id = MIDI_VOLUME_CC7; 
				break;
			case kCtrlPan: 
				id = MIDI_PAN_CC10; 
				break;
			case kCtrlExpression: 
				id = MIDI_EXPRESSION_CC11; 
				break;
			case kAfterTouch: 
				id = MIDI_CHANNEL_PRESSURE; 
				break;
			case kCtrlSustainOnOff: 
				id = MIDI_SUSTAIN_PEDAL; 
				break;
			case kCtrlAllNotesOff: 
				id = MIDI_ALL_NOTES_OFF; 
				break;
		}
		if (id == -1)
		{
			id = 0;
			return kResultFalse;
		}
		else
			return kResultTrue;
	}

	return kResultFalse;
}

/* 
	Controller::createView()
	Create our VSTGUI editor from the UIDESC file.
*/
IPlugView* PLUGIN_API Controller::createView(FIDString _name)
{
	ConstString name(_name);
	if(name == ViewType::kEditor)
	{
		// --- create the editor using the .uidesc file (in our Resources, edited with WYSIWYG editor in client)
		return new VSTGUI::VST3Editor (this, "Editor", "vstgui.uidesc");
	}
	return 0;
}
}}} // namespaces
