#pragma once
#include "mdaBaseController.h"
#include "mdaParameter.h"
#include "helpers.h"
#include "editorLayout.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"
#include <algorithm>
#include <cstring>
#include "scopeCapture.h"
#include "presetBarView.h"
#include "vstgui/uidescription/uiattributes.h"
#include <cmath>
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
			CRect viewRect(0, 0, ::editorLayout::kEditorWidth, ::editorLayout::kEditorHeight);
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
	if (tag < GPARAMS)
		editingParam[tag] = true;
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



		if (tag != MASTER_VOL && tag != POLY_COUNT)
			presetDirty = true;  // preset bar: the sound was edited since it was loaded
		// B-62: this return had ended up inside the comment above (c940a82), so no GUI edit reached the host
		return EditController::performEdit(tag, valueNormalized);
	}
	// B-07: step buttons are not host parameters; this path used to fall off the end of a
	// non-void function (undefined behaviour).
	return kResultFalse;
}


//------------------------------------------------------------------------
tresult BaseController::endEdit (ParamID tag)
{
	if (tag < GPARAMS)
		editingParam[tag] = false;
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

	if (message->getMessageID() && message->getMessageID()[0] == PMSG_SCOPE_CHAR
		&& message->getMessageID()[1] == 0) {
		const void* data = nullptr;
		uint32 size = 0;
		if (message->getAttributes()->getBinary("data", data, size) == kResultTrue && data
			&& size == sizeof(scopeFrame)) {
			std::memcpy(scopeFrame, data, size);
			scopeFrames++;
		}
		return kResultTrue;
	}

	if (message->getMessageID() && message->getMessageID()[0] == PMSG_VOICES_CHAR
		&& message->getMessageID()[1] == 0) {
		const void* data = nullptr;
		uint32 size = 0;
		if (message->getAttributes()->getBinary("data", data, size) == kResultTrue && data
			&& size == sizeof(voiceStates))
			std::memcpy(voiceStates, data, size);
		return kResultTrue;
	}

	if (message->getMessageID() && message->getMessageID()[0] == PMSG_PLAYHEADS_CHAR
		&& message->getMessageID()[1] == 0) {
		const void* data = nullptr;
		uint32 size = 0;
		if (message->getAttributes()->getBinary("data", data, size) == kResultTrue && data
			&& size == sizeof(playheads)) {
			std::memcpy(playheads, data, size);
			playheadTime = std::chrono::steady_clock::now();
			playheadFrames++;
		}
		return kResultTrue;
	}

	if (message->getMessageID() && message->getMessageID()[0] == PMSG_STATE_CHAR
		&& message->getMessageID()[1] == 0) {
		const void* data = nullptr;
		uint32 size = 0;
		int64 purpose = 0;
		message->getAttributes()->getInt("purpose", purpose);
		if (message->getAttributes()->getBinary("data", data, size) == kResultTrue && data && size > 0)
			onProcessorState(data, size, purpose);
		return kResultTrue;
	}

	if (message->getMessageID() && message->getMessageID()[0] == PMSG_PEAK_CHAR
		&& message->getMessageID()[1] == 0) {
		double peak = 0.0;
		if (message->getAttributes()->getFloat("peak", peak) == kResultTrue) {
			outPeak = float(peak);
			peakFrames++;
		}
		return kResultTrue;
	}

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

	// MASTER_VOL is read from its tagged block after bypass (BaseProcessor::setState); older
	// states hold 0 in its array slot, so a missing block means the default (0 dB).
	auto applyMasterVol = [this] (ParamValue v) { setParamNormalized (MASTER_VOL, v); };
	// POLY_COUNT likewise, from the "POLY" block after it (2026-11-15)
	auto applyPoly = [this] (ParamValue v) { setParamNormalized (POLY_COUNT, v); };
	// parameters newer than the state's version get their defaults (constants.h)
	auto applyNewDefaults = [this] (uint32 version) {
		applyNewParamDefaults (version, [this] (int pid, double v) { setParamNormalized (pid, v); });
	};
	auto truncated = [&] () {
		applyMasterVol (DEFAULT_MASTER_VOL);
		applyPoly (polyToNormalized (POLY_DEFAULT));
		applyNewDefaults (1);
		return kResultFalse;
	};
	// read each parameter
	for (uint32 i = 0; i < temp; i++){
		ParamValue value;
		if (streamer.readDouble (value) == false)
			return truncated ();
		if (i == MASTER_VOL || i == POLY_COUNT)
			continue;

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
	if (streamer.readInt32u(bypassState) == false)
		return truncated ();

	{
		// tagged blocks after bypass, in any order: master volume, polyphony, state version
		double masterVol = DEFAULT_MASTER_VOL, poly = polyToNormalized (POLY_DEFAULT);
		uint32 version = 1;
		for (int k = 0; k < 32; k++) {
			uint32 tag = 0;
			double v = 0.0;
			if (!streamer.readInt32u (tag) || !streamer.readDouble (v))
				break;
			if (!std::isfinite (v))
				continue;
			if (tag == MASTER_VOL_STATE_TAG)
				masterVol = (std::min) (1.0, (std::max) (0.0, v));
			else if (tag == POLY_STATE_TAG)
				poly = (std::min) (1.0, (std::max) (0.0, v));
			else if (tag == STATE_VERSION_TAG && v >= 1.0 && v < 1.0e6)
				version = static_cast<uint32> (v);
		}
		applyMasterVol (masterVol);
		applyPoly (poly);
		applyNewDefaults (version);
	}
		
	Parameter* bypassParam = parameters.getParameter (kBypassParam);
	if (bypassParam) {
		bypassParam->setNormalized(bypassState);
	}
		

	return kResultTrue;
}

static_assert(BaseController::kScopeLength == ScopeCapture::kLength, "scope frame size");

VSTGUI::CView* BaseController::createCustomView(VSTGUI::UTF8StringPtr name, const VSTGUI::UIAttributes& attributes,
	const VSTGUI::IUIDescription*, VSTGUI::VST3Editor*) {
	if (name && std::strcmp(name, "polyCount") == 0) {
		CPoint origin(polyZone::kRect.x, polyZone::kRect.y), size(polyZone::kRect.w, polyZone::kRect.h);
		attributes.getPointAttribute("origin", origin);
		attributes.getPointAttribute("size", size);
		return new PolyView(CRect(origin, size), this);
	}
	if (name && std::strcmp(name, "presetBar") == 0) {
		CPoint origin(presetBar::kBar.x, presetBar::kBar.y), size(presetBar::kBar.w, presetBar::kBar.h);
		attributes.getPointAttribute("origin", origin);
		attributes.getPointAttribute("size", size);
		return new PresetBarView(CRect(origin, size), this);
	}
	return nullptr;
}

PresetBarView::PresetBarView(const CRect& size, BaseController* c) : CView(size), ctrl(c) {
	setTransparency(true);
	setWantsFocus(false);
}

PresetBarView::~PresetBarView() {
	if (ctrl)
		ctrl->presetHover = -1;
}

int PresetBarView::zoneFor(const CPoint& where) const {
	// where is in the parent's coordinates, like getViewSize(); the zones are in window coordinates
	const CRect r = getViewSize();
	return presetBar::zoneAt(float(where.x - r.left) + presetBar::kBar.x, float(where.y - r.top) + presetBar::kBar.y);
}

CMouseEventResult PresetBarView::onMouseMoved(CPoint& where, const CButtonState&) {
	if (!ctrl)
		return kMouseEventNotHandled;
	ctrl->presetHover = ctrl->presetBarEnabled ? zoneFor(where) : -1;
	return kMouseEventHandled;
}

CMouseEventResult PresetBarView::onMouseExited(CPoint&, const CButtonState&) {
	if (ctrl)
		ctrl->presetHover = -1;
	return kMouseEventHandled;
}

CMouseEventResult PresetBarView::onMouseDown(CPoint& where, const CButtonState& buttons) {
	if (!ctrl || !ctrl->presetBarEnabled || !buttons.isLeftButton())
		return kMouseEventNotHandled;
	const int z = zoneFor(where);
	if (z < 0)
		return kMouseEventNotHandled;
	CPoint framePoint(where);
	localToFrame(framePoint);
	ctrl->onPresetBarClick(z, getFrame(), framePoint);
	return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}

// ---- polyphony control ----
void BaseController::setPolyCount(int n, bool withGesture) {
	n = n < 1 ? 1 : (n > POLY_MAX ? POLY_MAX : n);
	if (n == polyCount())
		return;
	const ParamValue v = polyToNormalized(n);
	if (withGesture)
		beginEdit(POLY_COUNT);
	setParamNormalized(POLY_COUNT, v);
	performEdit(POLY_COUNT, v);
	if (withGesture)
		endEdit(POLY_COUNT);
}

void BaseController::showPolyMenu(CFrame* frame, const CPoint& where) {
	if (!frame)
		return;
	static const int kValues[] = {1, 2, 4, 6, 8, 12, 16, 24, 32, 48, 64};
	auto menu = makeOwned<COptionMenu>();
	menu->addEntry("POLYPHONY", -1, CMenuItem::kTitle | CMenuItem::kDisabled);
	const int cur = polyCount();
	for (int n : kValues) {
		char t[32];
		snprintf(t, sizeof(t), "   %d %s", n, n == 1 ? "voice" : "voices");
		menu->addEntry(t, -1, n == cur ? CMenuItem::kChecked : CMenuItem::kNoFlags);
	}
	menu->popup(frame, where, [this](COptionMenu* m) {
		const int i = static_cast<int>(m->getLastResult()) - 1;  // entry 0 is the title
		if (i >= 0 && i < static_cast<int>(sizeof(kValues) / sizeof(kValues[0])))
			setPolyCount(kValues[i], true);
	});
}

PolyView::PolyView(const CRect& size, BaseController* c) : CView(size), ctrl(c) {
	setTransparency(true);
	setWantsFocus(false);
}

PolyView::~PolyView() {
	if (ctrl)
		ctrl->polyHover = false;
}

CMouseEventResult PolyView::onMouseEntered(CPoint&, const CButtonState&) {
	if (ctrl && ctrl->presetBarEnabled) {  // presetBarEnabled: false while the about window is open
		ctrl->polyHover = true;
		ctrl->showParamReadout(POLY_COUNT);
	}
	return kMouseEventHandled;
}

CMouseEventResult PolyView::onMouseExited(CPoint&, const CButtonState&) {
	if (ctrl && !dragging)
		ctrl->polyHover = false;
	return kMouseEventHandled;
}

CMouseEventResult PolyView::onMouseDown(CPoint& where, const CButtonState& buttons) {
	if (!ctrl || !ctrl->presetBarEnabled || !buttons.isLeftButton())
		return kMouseEventNotHandled;
	dragging = true;
	moved = false;
	startY = where.y;
	startN = ctrl->polyCount();
	ctrl->beginEdit(POLY_COUNT);
	return kMouseEventHandled;
}

CMouseEventResult PolyView::onMouseMoved(CPoint& where, const CButtonState&) {
	if (!ctrl)
		return kMouseEventNotHandled;
	if (dragging) {
		const double dy = startY - where.y;  // up = more voices, 4 px per voice
		if (std::fabs(dy) >= 3.0)
			moved = true;
		if (moved) {
			ctrl->setPolyCount(startN + static_cast<int>(dy / 4.0), false);
			ctrl->showParamReadout(POLY_COUNT);
		}
	}
	return kMouseEventHandled;
}

CMouseEventResult PolyView::onMouseUp(CPoint& where, const CButtonState&) {
	if (!ctrl || !dragging)
		return kMouseEventNotHandled;
	dragging = false;
	ctrl->endEdit(POLY_COUNT);
	if (!moved) {
		CPoint p(where);
		localToFrame(p);
		ctrl->showPolyMenu(getFrame(), p);
	}
	return kMouseEventHandled;
}

bool PolyView::onWheel(const CPoint&, const CMouseWheelAxis& axis, const float& distance, const CButtonState&) {
	if (!ctrl || !ctrl->presetBarEnabled || axis != kMouseWheelAxisY || distance == 0.0f)
		return false;
	ctrl->setPolyCount(ctrl->polyCount() + (distance > 0.0f ? 1 : -1), true);
	ctrl->showParamReadout(POLY_COUNT);
	return true;
}

void BaseController::setScopeActive(bool on) {
	IMessage* msg = allocateMessage();
	if (!msg)
		return;
	msg->setMessageID(msgIdStr(CMSG_SCOPE_ENABLE_CHAR));
	msg->getAttributes()->setInt("on", on ? 1 : 0);
	sendMessage(msg);
	msg->release();
	if (!on) {
		scopeFrames = 0;  // next open starts empty
		std::memset(voiceStates, 0, sizeof(voiceStates));
		for (auto& p : playheads) p.pitch = -1;
		playheadFrames = 0;
		outPeak = 0.0f;
		peakFrames = 0;
	}
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
