#pragma once
#include "mdaBaseController.h"
#include "mdaParameter.h"
#include "helpers.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"
#include "vstgui/uidescription/delegationcontroller.h"

#include "filter.h"

/*
#include "regReader.h"
#include <locale>
#include <codecvt>
#include "bcrypt/BCrypt.hpp"
*/

using namespace VSTGUI;
namespace Steinberg {
namespace Vst {
namespace rs {


	/*
	//MAKE OUR VIEWS
	IController* BaseController::createSubController(const char* name, const IUIDescription* description, VST3Editor* editor){
		if (strcmp(name, "nvgViewController") == 0) {
			ParameterContainer& pcr = getParameters();
			//nvgViewController = new NVGViewController(editor, pcr);
			//std::unique_ptr<NVGViewController>nvgViewController(new NVGViewController(editor));
			//nvgViewController = std::unique_ptr<NVGViewController>(new NVGViewController(editor));

			//nvgViewController = new NVGViewController(editor);
			NVGViewController nvgViewController(editor);
			//nvgViewController->nvgView->baseController = this;
			nvgViewController.nvgView->lfo1Rate = &lfo4Rate;
			nvgViewController.nvgView->lfo2Rate = &lfo5Rate;
			nvgViewController.nvgView->lfo3Rate = &lfo6Rate;

			for (int i = 0; i < PARTPARAMS; i++) {
				nvgViewController.nvgView->NVGParamDef[i] = ParamDef[i];
			}
			if (constants::isRegLegit == true) {
				nvgViewController.nvgView->badReg = false;
			}
			else {
				nvgViewController.nvgView->badReg = true;
			}
			return &nvgViewController;
		
		}else {
			return 0;
		}
	}

	//MAKE OUR EDITOR
	IPlugView* PLUGIN_API BaseController::createView(FIDString _name) {
		ConstString name(_name);
		if (name == ViewType::kEditor) {
			// --- create the editor using the .uidesc file (in our Resources, edited with WYSIWYG editor in client)
			//
			CRect viewRect(0, 0, 1148, 542);
			rsView = new RSView(this, "Editor", "vstgui.uidesc", viewRect);
			
			return rsView;
		}
		return 0;
	}
	*/



	tresult PLUGIN_API BaseController::initialize (FUnknown* context){
		viewWidth = 1150.0;
		viewHeight = 550;
		tresult res = EditControllerEx1::initialize(context);
		
		//rsView = new RSView(this, "Editor", "vstgui.uidesc", viewRect);
		//paramatersPtr = &paramaters;
		if (res == kResultOk){
			UnitInfo uinfo;
			uinfo.id = kRootUnitId;
			uinfo.parentUnitId = kNoParentUnitId;
			uinfo.programListId = kPresetParam;
			UString name(uinfo.name, 128);
			name.fromAscii("Root");
			addUnit(new Unit(uinfo));


			if (addBypassParameter){
				IndexedParameter* bypassParam = new IndexedParameter(USTRING("Bypass"), 0, 1, 0, ParameterInfo::kIsBypass | ParameterInfo::kCanAutomate,
					kBypassParam);
				bypassParam->setIndexString(0, UString128("off"));
				bypassParam->setIndexString(1, UString128("on"));
				parameters.addParameter(bypassParam);
			}

			

		}
		//FIGURE OUT WHAT pPEAKPARAM DOES IN OG EXAMPLE
		/*
		tresult res = EditController::initialize (context);
		if (res == kResultTrue)
		{
			// add processing parameters
			PeakParameter* peakParam = new PeakParameter (ParameterInfo::kIsReadOnly, kPeakParam, USTRING("Peak"));
			parameters.addParameter (peakParam);
		}
		*/
		/*
		std::string password = "gobots";
		std::string password2 = "notgobots";
		std::wstring regSubKey;
		regSubKey = L"Software\\Rogue-Syntax\\Registrations\\DonnaMedina\\";

		std::wstring regValue(L"RSRegKey");
		try {
			valueFromRegistry = GetStringValueFromHKLM(regSubKey, regValue);
		}
		catch (std::exception& e) {
			regErr1 = e.what();
		}
		if (valueFromRegistry.size() > 0) {
			regKeyFound = true;
		}
		using convert_type = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_type, wchar_t> converter;
		std::string hashd = converter.to_bytes(valueFromRegistry);

		bool regIsValid = BCrypt::validatePassword(password2, hashd);
		if (regIsValid == true) {
			regKeyLegit = true;
			//used in DX10Processor::noteOn
		}
		else {
			//nvgViewController->nvgView->onBadReg(pid, 1.0);
			//this is null at this point
		}
		*/
		return res;
	}

//------------------------------------------------------------------------
Parameter* BaseController::getParameterObject (ParamID tag)
{
	Parameter* param = EditController::getParameterObject (tag);
	if (param == 0)
	{
		param = uiParameters.getParameter (tag);
	}
	return param;
}

// make sure that our UI only parameters doesn't call the following three EditController methods: beginEdit, endEdit, performEdit
//------------------------------------------------------------------------
tresult BaseController::beginEdit (ParamID tag)
{
	if (EditController::getParameterObject (tag))
		if (tag < STEP_BUTTONS_FIRST) {
			return EditController::beginEdit(tag);
		}
	return kResultFalse;
}


//------------------------------------------------------------------------
tresult BaseController::performEdit (ParamID tag, ParamValue valueNormalized)
{ 
	//if (isViewReady() == false) {
	//	return kResultFalse;
	//}
	if (tag < STEP_BUTTONS_FIRST) {
		if (EditController::getParameterObject(tag))
			if (linkTune == 1) {
				if (tag == TUNE_) {
					performEdit(MOD_TUNE_, valueNormalized);
					//tag = MOD_TUNE_;
				}
				if (tag == FINE_) {
					performEdit(MOD_FINE_, valueNormalized);
					//tag = MOD_FINE_;
				}
				if (tag == FINE_ENV_SRC) {
					performEdit(MOD_FINE_ENV_SRC, valueNormalized);
					//tag = MOD_FINE_;
				}
				if (tag == TUNE_ENV_SRC) {
					performEdit(MOD_TUNE_ENV_SRC, valueNormalized);
					//tag = MOD_FINE_;
				}
				if (tag == FINE_ENV) {
					performEdit(MOD_FINE_ENV, valueNormalized);
					//tag = MOD_FINE_;
				}
				if (tag == TUNE_ENV) {
					performEdit(MOD_TUNE_ENV, valueNormalized);
					//tag = MOD_FINE_;
				}
			}



		return EditController::performEdit(tag, valueNormalized);
	}
	// B-07: step buttons are not host parameters; this path used to fall off the end of a
	// non-void function (undefined behaviour).
	return kResultFalse;
}


//------------------------------------------------------------------------
tresult BaseController::endEdit (ParamID tag)
{
	if (EditController::getParameterObject (tag))
		if (tag < STEP_BUTTONS_FIRST) {
			return EditController::endEdit(tag);
		}
	return kResultFalse;
}


//------------------------------------------------------------------------
bool BaseController::isPrivateParameter (const Steinberg::Vst::ParamID paramID)
{
	return uiParameters.getParameter (paramID) != 0 ? true : false;
}


const TChar BaseController::kMicroSecondsString[] = {0x00b5, 0x0073, 0x0};





//-----------------------------------------------------------------------------
int32 PLUGIN_API BaseController::getProgramListCount (){ 
	if (parameters.getParameter (kPresetParam))
		return 1;
	return 0;
}


//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseController::getProgramListInfo (int32 listIndex, ProgramListInfo& info /*out*/){ 
	Parameter* param = parameters.getParameter (kPresetParam);
	if (param && listIndex == 0)
	{
		info.id = kPresetParam;
		info.programCount = (int32)param->toPlain (1) + 1;
		UString name (info.name, 128);
		name.fromAscii ("Presets");
		return kResultTrue;
	}
	return kResultFalse;
}


void BaseController::addPIDParameter(char* titler, char* units,
	int32 stepCount, ParamValue defaultNormalizedValue,
	int32 flags, int32 tag) {

	parameters.addParameter(USTRING(titler), USTRING(units), stepCount, defaultNormalizedValue, flags, tag);
	char* title = titler;
	pidMap[title] = tag;

	if (tag < NPARAMS) {
		
	}

}


void BaseController::addPIDParameter(char* titler, char* units, int32 stepCount, ParamValue defaultNormalizedValue,	int32 flags, int32 tag,int min, int max, bool isTrue) {

	parameters.addParameter(new ScaledParameter(USTRING(titler), USTRING(units), stepCount, defaultNormalizedValue, flags, tag, min, max, isTrue));

	char* title = titler;
	pidMap[title] = tag;
	
	if (tag < NPARAMS) {
		
	}

}


//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseController::getProgramName (ProgramListID listId, int32 programIndex,
                                                   String128 name /*out*/)
{
	if (listId == kPresetParam)
	{
		Parameter* param = parameters.getParameter (kPresetParam);
		if (param)
		{
			ParamValue normalized = param->toNormalized (programIndex);
			param->toString (normalized, name);
			return kResultTrue;
		}
	}
	return kResultFalse;
}


std::u16string BaseController::StringtoU16(const std::string &str) {
	std::u16string wstr = u"";
	char16_t c16str[3] = u"\0";
	mbstate_t mbs;
	for (const auto& it : str) {
		memset(&mbs, 0, sizeof(mbs));//set shift state to the initial state
		memmove(c16str, u"\0\0\0", 3);
		mbrtoc16(c16str, &it, 3, &mbs);
		wstr.append(std::u16string(c16str));
	}//for
	return wstr;
}

tresult PLUGIN_API BaseController::sendParamKeys() {
	/*
	for (std::pair<char*, int> param : pidMap) {
		IMessage* msg = allocateMessage();
		if (msg) {

			UString128 tmp;
			tmp.fromAscii(param.first, 128);
			msg->setMessageID("paramKeys");
			msg->getAttributes()->setString("name", tmp );
			msg->getAttributes()->setInt("id", param.second);
			sendMessage(msg);
			msg->release();
		}

	}
	*/
	
	return kResultTrue;

}

//-----------------------------------------------------------------------------
BaseController::BaseController() : sampleRate(44100), addBypassParameter(true)
{
	for (int32 i = 0; i < kCountCtrlNumber; i++) {
		midiCCParamID[i] = -1;
	}

	onNotif[CMSG_GET_SAMPLE_RATE] = &BaseController::cmsgOnGetSampleRate;
	onNotif[CMSG_LINK_TUNE] = &BaseController::cmsgOnLinkCtrl;

}


int BaseController::cmsgOnGetSampleRate(IMessage* message) {
	message->getAttributes()->getFloat("SampleRate", sampleRate);

	return kResultTrue;
}
/*
int BaseController::cmsgOnStepSwitch(IMessage* message) {

	return 0;
}
*/
int BaseController::cmsgOnLinkCtrl(IMessage* message) {
	message->getAttributes()->getInt("0", linkTune);
	return 0;
}


//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseController::notify (IMessage* message){

	//IConnectionPoint* ip = getPeer();
	int64 pid;
	double value;
	int64 messageID = -1;

	if (strcmp(message->getMessageID(), "InstanceID") == 0) {
		message->getAttributes()->getInt("id", this->myInstanceId);
		return kResultTrue;
	}

	if (message->getMessageID()) {

		msgInt = int(*message->getMessageID());

		message->getAttributes()->getInt("id", messageID);
		if (messageID != this->myInstanceId && messageID != -1 ) {
			return kResultFalse;
		}

		switch (msgInt) {
		case PMSG_STEP_CTRL_CHAR:
			message->getAttributes()->getInt("pid", pid);
			message->getAttributes()->getFloat("val", value);
			//onStepCtrlButton(pid, value);
			return kResultTrue;
			break;
		case CMSG_GET_SAMPLE_RATE_CHAR:
			message->getAttributes()->getFloat("SampleRate", sampleRate);
			return kResultTrue;
			break;
		case CMSG_LINK_TUNE_CHAR:
			message->getAttributes()->getInt("0", linkTune);
			return kResultTrue;
			break;
		case PMSG_LFO4_RATE_CHAR:
			message->getAttributes()->getFloat("0", lfo4Rate);
			return kResultTrue;
			break;
		case PMSG_LFO5_RATE_CHAR:
			message->getAttributes()->getFloat("0", lfo5Rate);
			return kResultTrue;
			break;
		case PMSG_LFO6_RATE_CHAR:
			message->getAttributes()->getFloat("0", lfo6Rate);
			return kResultTrue;
			break;
		case PMSG_STEP_SWITCH2_CHAR:
			//int64 pid;
			//double value;
			message->getAttributes()->getInt("pid", pid);
			message->getAttributes()->getFloat("val", value);
			doFromBase(pid, value);
			EditController::getParamStringByValue(pid, value, stringCtrlTChar);
			stringCtrlWString = stringCtrlTChar;
			stringCtrlInt = safeStoi(&stringCtrlWString);

			IMessage* stringCtrlMsg = allocateMessage();

			if (stringCtrlMsg) {
				stringCtrlMsg->setMessageID(msgIdStr(PMSG_STEP_SWITCH_CHAR));  // B-27
				stringCtrlMsg->getAttributes()->setInt("0", stringCtrlInt);
				stringCtrlMsg->getAttributes()->setInt("1", pid);
				stringCtrlMsg->getAttributes()->setInt("id", this->myInstanceId);
				sendMessage(stringCtrlMsg);
				stringCtrlMsg->release();
			}
			return kResultTrue;
			break;


		}
	}
	else {
		return kResultFalse;
	}
	/*
	if (message->getMessageID == CMSG_ON_GET_SAMPLE_RATE){
		message->getAttributes ()->getFloat ("SampleRate", sampleRate);

		return kResultTrue;
	}
	*/

	/*
	if (message->getMessageID() == "2") {
		//message 1 - retrieve string value from string swicth 
		Steinberg::int64 pidIn = 0;
		message->getAttributes()->getInt("p", pidIn);//sets value at (*output)
		EditController::getParameterObject(pidIn)->getNormalized();
		Steinberg::Vst::TChar* tempValue = new Steinberg::Vst::TChar();
		EditController::getParamStringByValue(pidIn, EditController::getParameterObject(pidIn)->getNormalized(), tempValue);
		IMessage* msg = allocateMessage();
		int tempInt = static_cast<int>((*tempValue));
		if (msg) {
			msg->setMessageID("2");
			msg->getAttributes()->setInt("0", tempInt);
			sendMessage(msg);
			msg->release();
		}
		return kResultTrue;
	}
	else if (strcmp(message->getMessageID(), "si") == 0) {

		message->getAttributes()->getFloat("o", sinLev);//sets value at (*output)
		Parameter* param = parameters.getParameter(49);
		if (param) {
			//param->setNormalized(param->toNormalized(sinLev));
			//NOT WORKING HERE, TRY ON CTRL INPUT
		}
		return kResultTrue;
	}
	else if (strcmp(message->getMessageID(), "output") == 0 ) {
		
		message->getAttributes()->getFloat("output", output);//sets value at (*output)
		//(*rsView).sfmlView.setOutputD(output);
		//sfmlViewController->sfmlViewOsc->setOutputD(output);
		//sfmlViewController->sfmlViewNvg->setOutputD(output);
		return kResultTrue;
	}
	else if (strcmp(message->getMessageID(), "env") == 0) {

		message->getAttributes()->getFloat("envVal", envVal);//sets value at (*output)
		//(*rsView).sfmlView.setOutputD(output);
		//sfmlViewController->sfmlViewOsc->setOutputD(output);
		//sfmlViewController->sfmlViewNvg->setOutputD(output);
		return kResultTrue;
	}
	*/
	
	return EditControllerEx1::notify (message);
}

int  BaseController::safeStoi(std::wstring* Str, size_t *_Idx, int _Base) {	// convert wstring to int
	int& _Errno_ref = errno;	// Nonzero cost, pay it once
	const wchar_t *_Ptr = Str->c_str();
	wchar_t *_Eptr;
	_Errno_ref = 0;
	const long _Ans = _CSTD wcstol(_Ptr, &_Eptr, _Base);
	if (_Idx != nullptr)
		*_Idx = (size_t)(_Eptr - _Ptr);
	return ((int)_Ans);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseController::setComponentState (IBStream* state)
{
	if (!state) {
		return kResultFalse;
	}
		
	IBStreamer streamer (state, kLittleEndian);
	uint32 temp;
	streamer.readInt32u (temp); // numParams or Header

	if (temp == BaseController::kMagicNumber) {
		// read current Program
		streamer.readInt32u(temp);
		Parameter* param = parameters.getParameter(kPresetParam);
		if (param){
			param->setNormalized(param->toNormalized(temp));
		}
		// numParams
		streamer.readInt32u (temp);
	}

	// read each parameter
	for (uint32 i = 0; i < temp; i++){
		ParamValue value;
		if (streamer.readDouble (value) == false)
			return kResultFalse;

		if (nvgViewReady) {
			// Apply the parameter directly if the view is ready
			setParamNormalized(i, value);
			if (i == MOD_TYPE_) {
				doFromBase(i, value);
			}
			if (i == LFO_1_BPM) {
				doFromBase(i, value);
			}
			if (i == LFO_2_BPM) {
				doFromBase(i, value);
			}
			if (i == LFO_3_BPM) {
				doFromBase(i, value);
			}
			if (i == SYNC_GEN_MOD_TUNE) {
				doFromBase(i, value);
			}
		}
		else {
			// B-02: store the value now (it used to be only queued, so the controller reported
			// defaults until the editor was opened); the derived controller queues the GUI update.
			setParamNormalized(i, value);
			hasPendingState = true;
		}
		
	}

	// bypass
	uint32 bypassState;
	if (streamer.readInt32u(bypassState) == false) {
		return kResultFalse;
	}
		
	Parameter* bypassParam = parameters.getParameter (kBypassParam);
	if (bypassParam) {
		bypassParam->setNormalized(bypassState);
	}
		

	return kResultTrue;
}

void BaseController::onNVGViewReady() {

	// Apply all pending updates
	for (const auto& pair  : pendingStateUpdates) {
		// NEEDS TO MATCH WHAT IN SET COMPONENT STATE
		int i = pair.first;
		ParamValue value = pair.second;
		setParamNormalized(i, value);
		if (i == MOD_TYPE_) {
			doFromBase(i, value);
		}
		if (i == LFO_1_BPM) {
			doFromBase(i, value);
		}
		if (i == LFO_2_BPM) {
			doFromBase(i, value);
		}
		if (i == LFO_3_BPM) {
			doFromBase(i, value);
		}
		if (i == SYNC_GEN_MOD_TUNE) {
			doFromBase(i, value);
		}
	}

	// Clear the pending updates
	pendingStateUpdates.clear();
	hasPendingState = false;
}

//------------------------------------------------------------------------
tresult PLUGIN_API BaseController::getMidiControllerAssignment (int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& tag /*out*/){
	//NOT IMPLEMENTING VIEW CHECK HERE
	if (busIndex == 0 && midiControllerNumber < kCountCtrlNumber && midiCCParamID[midiControllerNumber] != -1){
		tag = midiCCParamID[midiControllerNumber];
		return kResultTrue;
	}
	return kResultFalse;
}


//------------------------------------------------------------------------
tresult PLUGIN_API BaseController::queryInterface (const char* iid, void** obj)
{
	QUERY_INTERFACE (iid, obj, IMidiMapping::iid, IMidiMapping)
	return EditControllerEx1::queryInterface (iid, obj);
}





/*


//I THINK I CAN DEL THIS
	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	class PeakParameter : public Parameter
	{
	public:
		PeakParameter(int32 flags, ParamID id, const TChar* title);

		void toString(ParamValue normValue, String128 string) const override;
		bool fromString(const TChar* string, ParamValue& normValue) const override;
	};

	//------------------------------------------------------------------------
	PeakParameter::PeakParameter(int32 flags, ParamID id, const TChar* title)
	{
		UString(info.title, USTRINGSIZE(info.title)).assign(title);

		info.flags = flags;
		info.id = id;
		info.stepCount = 0;
		info.defaultNormalizedValue = 0.5f;
		info.unitId = kRootUnitId;

		setNormalized(1.f);
	}

	//------------------------------------------------------------------------
	void PeakParameter::toString(ParamValue normValue, String128 string) const
	{
		Steinberg::String str;
		if (normValue > 0.0001)
		{
			str.printf("%.3f", 20 * log10f((float)normValue));
		}
		else
		{
			str.assign("-");
			str.append(kInfiniteSymbol);
		}
		str.toWideString(kCP_Utf8);
		str.copyTo16(string, 0, 128);
	}

	//------------------------------------------------------------------------
	bool PeakParameter::fromString(const TChar* string, ParamValue& normValue) const
	{
		return false;
	}

	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	//------------------------------------------------------------------------

	*/



}
}
} // namespaces
