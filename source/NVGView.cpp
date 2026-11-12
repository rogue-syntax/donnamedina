#pragma once
//#include <SFML/System/Thread.hpp>
//or needs threading
#include "REvent.h"
#include "NVGView.h"
#include <iostream>
#include "luconData.hpp"
#include "constants.h"
#include "glIncludes.h"

using namespace VSTGUI;
namespace Steinberg {
namespace Vst {
namespace rs {



	void NVGView::getViewAttr(CView* view, IdStringPtr attrName, std::string& value) {
		//const char* origin("origin");
		//static std::hash<std::string> hashFunc;
		size_t hash = hashFunc(attrName);
		uint32_t getAttrSize = 0;
		if ( (*view).getAttributeSize(hash, getAttrSize) ){
			getAttrTemp = new char[getAttrSize];
			if ( (*view).getAttribute(hash, getAttrSize, getAttrTemp, getAttrSize) ){
				value = getAttrTemp;
			}
		}
		
	}

	void NVGView::drawBlank(RData& obj) {

	}
	/*
	why is pid 153 being trhrown for width anim @  NVGView::initResetWidthAnim
no viewindex at 153!*/
	void NVGView::initResetWidthAnim(int pid) {
		if (viewsReady[pid] == true ) {
			(*viewIndex[pid]).blinkIntensity = 0.0;
			(*viewIndex[pid]).blinker = 0.0;
			viewIndex[pid]->blinkerDone = false;
		}
		
		
	}

	void NVGView::onBadReg(int pid, double msgc) {
		//badReg = true;
	}

	void NVGView::onEnv(int pid, std::string msgc) {
		try {
			textDisplayDecay = 1.0f;
			targDisplayTxt.reset(NVGParamDef[pid].c_str(), NVGParamDef[pid], 0.025f, 0.025f);
			targValueTxt.reset(msgc.c_str(), NVGParamDef[pid], 0.013f, 0.013f);
		
		}
		catch (const std::exception& e) {

		}
		
	}

	void NVGView::onEnvBipolar(int pid, std::string msgc) {
		try {
			textDisplayDecay = 1.0f;
			targDisplayTxt.reset(envNameMap[pid], envNameMap[pid], 0.025f, 0.025f);
			targValueTxt.reset(msgc.c_str(), envNameMap[pid], 0.013f, 0.013f);
		}
		catch (const std::exception& e) {

		}
	
	}

	void NVGView::onPanEnv(int pid, std::string msgc) {
		try {
			textDisplayDecay = 1.0f;
			//selectedCtrlTarg = NVGParamDef[pid];
			//selectedCtrlValue = msgc;
			const char* c = "";
			c = msgc.c_str();
			targDisplayTxt.reset(NVGParamDef[pid].c_str(), NVGParamDef[pid], 0.025f, 0.025f);
			targValueTxt.reset(c, NVGParamDef[pid], 0.013f, 0.013f);
		}
		catch (const std::exception& e) {

		}
		
	}

	void NVGView::onEnvLev(int pid, std::string msgc) {
		try {
			targDisplayTxt.reset(envNameMap[pid], envNameMap[pid], 0.025f, 0.025f);
			targValueTxt.reset(msgc.c_str(), envNameMap[pid], 0.013f, 0.013f);
		}
		catch (const std::exception& e) {

		}
		//textDisplayDecay = 1.0f;
		//selectedCtrlTarg = envNameMap[pid];
		

	}

	void NVGView::onTuneSwitch(int pid, std::string interv) { 
		try {
			targDisplayTxt.reset(NVGParamDef[pid].c_str(), NVGParamDef[pid], 0.025f, 0.025f);
			targValueTxt.reset(interv.c_str(), NVGParamDef[pid], 0.013f, 0.013f);

		}
		catch (const std::exception& e) {

		}
		
	}




	void NVGView::onOnOffBtn(int pid, int state) {
		try {
			textDisplayDecay = 1.0f;
			//selectedCtrlTarg = envNameMap[pid];
			targDisplayTxt.reset(envNameMap[pid], envNameMap[pid], 00.025f, 0.025f);
			switch (state) {
			case 0:
				//selectedCtrlValue = "OFF";
				targValueTxt.reset("OFF", envNameMap[pid], 0.013f, 0.013f);
				break;
			case 1:
				targValueTxt.reset("ON", envNameMap[pid], 0.013f, 0.013f);
				break;
			}
		}
		catch (const std::exception& e) {

		}
		
	}

	void NVGView::onEnvSrc(int pid, int envSrc) {
		try {
			textDisplayDecay = 1.0f;

			targDisplayTxt.reset(NVGParamDef[pid].c_str(), NVGParamDef[pid], 0.025f, 0.025f);
			targValueTxt.reset(envSrcNames[envSrc], NVGParamDef[pid], 0.013f, 0.013f);
		}
		catch (const std::exception& e) {

		}
	
	}

	void NVGView::onChangeOSCType(int pid, int envSrc) {
		//selectedCtrlTarg = NVGParamDef[pid];
		try {
			targDisplayTxt.reset(NVGParamDef[pid].c_str(), NVGParamDef[pid], 0.025f, 0.025f);
			textDisplayDecay = 1.0f;
			switch (envSrc) {
			case 0:
				//selectedCtrlValue = "SINE";
				targValueTxt.reset("SINE", NVGParamDef[pid], 0.013f, 0.013f);
				break;
			case 1:
				//selectedCtrlValue = "SAW";
				targValueTxt.reset("SAW", NVGParamDef[pid], 0.013f, 0.013f);
				break;
			case 2:
				//selectedCtrlValue = "TRI";
				targValueTxt.reset("TRI", NVGParamDef[pid], 0.013f, 0.013f);
				break;
			case 3:
				//selectedCtrlValue = "SQUARE";
				targValueTxt.reset("SQUARE", NVGParamDef[pid], 0.013f, 0.013f);
				break;
			case 4:
				targValueTxt.reset("SQUARE", NVGParamDef[pid], 0.013f, 0.013f);
				//selectedCtrlValue = "SQUARE";
				break;
			}
		}
		catch (const std::exception& e) {

		}
		
	}

	void NVGView::onChangeLFOOSCType(int pid, int envSrc) {
		//selectedCtrlTarg = envNameMap[pid];
		try {
			targDisplayTxt.reset(envNameMap[pid], envNameMap[pid], 0.025f, 0.025f);
			textDisplayDecay = 1.0f;
			switch (envSrc) {
			case 0:
				targValueTxt.reset("SINE", envNameMap[pid], 0.013f, 0.013f);
				//selectedCtrlValue = "SINE";
				break;
			case 1:
				//selectedCtrlValue = "SAW";
				targValueTxt.reset("SAW", envNameMap[pid], 0.013f, 0.013f);
				break;
			case 2:
				//selectedCtrlValue = "TRI";
				targValueTxt.reset("TRI", envNameMap[pid], 0.013f, 0.013f);
				break;
			case 3:
				targValueTxt.reset("SQUARE", envNameMap[pid], 0.013f, 0.013f);
				//selectedCtrlValue = "SQUARE";
				break;
			case 4:
				targValueTxt.reset("SQUARE", envNameMap[pid], 0.013f, 0.013f);
				// = "SQUARE";
				break;
			}
		}
		catch (const std::exception& e) {

		}
		
	}

	void NVGView::onChangeModType(int pid, int envSrc) {
		try {
			selectedCtrlPID = pid;
			//selectedCtrlTarg = NVGParamDef[pid];
			targDisplayTxt.reset(NVGParamDef[pid].c_str(), NVGParamDef[pid], 0.025f, 0.025f);
			textDisplayDecay = 1.0f;
			switch (envSrc) {
			case 0:
				//selectedCtrlValue = "";
				targValueTxt.reset("", NVGParamDef[pid], 0.013f, 0.013f);
				beatModOn = 0.0f;
				beatModOnLev = 0.0f;
				beatModOnPrev = 0.0f;
				break;
			case 1:
				//selectedCtrlValue = "PHASE MOD";
				targValueTxt.reset("PHASE MOD", NVGParamDef[pid], 0.013f, 0.013f);
				beatModOn = 0.0f;
				beatModOnLev = 0.0f;
				beatModOnPrev = 0.0f;
				break;
			case 2:
				//selectedCtrlValue = "FREQ MOD";
				targValueTxt.reset("FREQ MOD", NVGParamDef[pid], 0.013f, 0.013f);
				beatModOn = 0.0f;
				beatModOnLev = 0.0f;
				beatModOnPrev = 0.0f;
				break;
			case 3:
				targValueTxt.reset("DETUNED BEAT MOD", NVGParamDef[pid], 0.013f, 0.013f);
				//selectedCtrlValue = "DETUNED BEAT MOD";
				beatModOn = 1.0f;
				if (beatModOnPrev == 0.0f) {
					beatModOnLev = 0.0f;
				}
				beatModOnPrev = 1.0f;
				break;

			}
		}
		catch (const std::exception& e) {

		}
	
	}

	
	void NVGView::onResetButton(int pid) {
		try {
			int idx = pid - WIDTH_RESET;//relatonship between RESET pids and target ctrlsi.e. WIDTH, TUNE, etc
			if (controlMap[resetSwitch[idx]] == nullptr) {
				return;
			}
			controlMap[resetSwitch[idx]]->setValueNormalized(0.5f);//THIS RESETS THE VALUE ON THE ACTUAL CONTROL
			controlMap[resetSwitch[idx]]->valueChanged();//THIS RESETS THE VALUE ON THE ACTUAL CONTROL
			Anima ar;
			ar.animName = BUTTON_BLINK_;
			ar.controlIdx = pid;
			ar.animState = 0;
			for (auto it = animationReel.begin(); it != animationReel.end(); ++it) {
				if (it->animName == ar.animName && it->controlIdx == ar.controlIdx) {
					animationReel.erase(it);
					break;
				}
			}
			initResetWidthAnim(pid);
			animationReel.push_back(ar);
		}
		catch (const std::exception& e) {

		}
		
	}

	void NVGView::onStepCtrlButton(int pid) {
		//controlMap[pid]->setValueNormalized(0.0f);//THIS RESETS THE VALUE ON THE ACTUAL CONTROL
		//controlMap[pid]->valueChanged();//THIS RESETS THE VALUE ON THE ACTUAL CONTROL
		try {
			Anima ar;
			ar.animName = BUTTON_BLINK_FASTER;
			ar.controlIdx = pid;
			ar.animState = 0;
			for (auto it = animationReel.begin(); it != animationReel.end(); ++it) {
				if (it->animName == ar.animName && it->controlIdx == ar.controlIdx) {
					animationReel.erase(it);
					break;
				}
			}
			initResetWidthAnim(pid);
			animationReel.push_back(ar);
		}
		catch (const std::exception& e) {

		}
		
	}

	void NVGView::onStepCtrlSwitch(int pid) {
		//controlMap[pid]->setValueNormalized(0.0f);//THIS RESETS THE VALUE ON THE ACTUAL CONTROL
		//controlMap[pid]->valueChanged();//THIS RESETS THE VALUE ON THE ACTUAL CONTROL
		try {
			if (stepCtrlSelectAnim.controlIdx != NULL) {
				(*viewIndex[stepCtrlSelectAnim.controlIdx]).blinkIntensity = 0.0f;
			}

			stepCtrlSelectAnim.animName = BUTTON_BLINK_FASTER;
			stepCtrlSelectAnim.controlIdx = pid;
			stepCtrlSelectAnim.animState = 0;
			selectedStepTarg = viewIndex[stepCtrlSelectAnim.controlIdx];
			if (pid < 100) {
				selectedStepTarg->RSBType = 1;
			}
			else {
				selectedStepTarg->RSBType = 2;
			}
			initResetWidthAnim(pid);

			textDisplayDecay = 1.0f;

			targDisplayTxt.reset("STEP_CTRL_TARGET", "STEP_CTRL_TARGET", 0.025f, 0.025f);

			if (pid > 100) {
	
				targValueTxt.reset(envNameMap[pid], "STEP_CTRL_TARGET", 0.013f, 0.013f);
			}
			else {
		
				targValueTxt.reset(NVGParamDef[pid].c_str(), "STEP_CTRL_TARGET", 0.013f, 0.013f);
			}

		}
		catch (const std::exception& e) {

		}
	}
	

	void NVGView::onToggleButton(int pid) {
		try {
			Anima ar;
			ar.animName = BUTTON_GLOW_;
			ar.controlIdx = pid;
			ar.animState = 0;
			for (auto it = animationReel.begin(); it != animationReel.end(); ++it) {
				if (it->animName == ar.animName && it->controlIdx == ar.controlIdx) {
					animationReel.erase(it);
					break;
				}
			}
			initResetWidthAnim(pid);
			animationReel.push_back(ar);
		}
		catch (const std::exception& e) {

		}
		
		
	}

	void NVGView::onRCButtonUp( RDataConsumer* rdc) {
		try {
			knobMap.at("width")->setValueNormalized(0.5f);
		}
		catch (const std::exception& e) {

		}
		
		

		/*
		

		addAnimation("blinkButton", this, new Animation::RepeatTimingFunction(new Animation::LinearTimingFunction(1000), 0, false));
		*/
	};


	



	void NVGView::setupRButton() {
		CBButton* cb = dynamic_cast<CBButton*> ((*viewListIter).get());
		buttonMap.emplace((*viewListIter).get()->customRSViewName, cb);//not used for drawing?

		RSBView newBView((*viewListIter).get()->customRSViewName);
		newBView.type = (*viewListIter).get()->customRSType;//TEST
		newBView.value = (*buttonMap[(*viewListIter).get()->customRSViewName]).getValue();
		newBView.drawRect = CRect((*viewListIter).get()->getViewSize());
		newBView.baseX = (*viewListIter).get()->getViewSize().getTopLeft().x;
		newBView.baseY = (*viewListIter).get()->getViewSize().getTopLeft().y;
		newBView.baseZ = 0;

		Draw.createRData((*viewListIter).get()->customRSViewName, newBView);
		Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawButton);
		RDrawVector[1].push_back((*viewListIter).get()->customRSViewName);
		RViewsMap.emplace((*viewListIter).get()->customRSViewName, newBView);

		viewIndex[(*buttonMap[(*viewListIter).get()->customRSViewName]).getTag()] = &RViewsMap.at((*viewListIter).get()->customRSViewName);
		viewsReady[(*buttonMap[(*viewListIter).get()->customRSViewName]).getTag()] = true;
		setupDisplayActive((*viewListIter).get());

		//EXAMPLE OF LATE BINDING GUI EVENTS 
		//requires cbbutton to be reventable
		//if ((*viewListIter).get()->customRSViewName == "linkGenerators1") {
		//	cb->fMap.updateContext("onMouseUp", this);
		//	cb->fMap.reBind("onMouseUp", this, &RDataConsumer::onLinkGen1Up);
		//}
		


	}

	//wtf
	void NVGView::valueChanged(CControl* pControl) {
		int pid = pControl->getTag();
		int idx = WIDTH_RESET - pid;
		controlMap[resetSwitch[idx]]->setValueNormalized(0.5f);
		Anima ar;
		ar.animName = BUTTON_BLINK_;
		ar.controlIdx = pid;
		ar.animState = 0;
		for (auto it = animationReel.begin(); it != animationReel.end(); ++it) {
			if (it->animName == ar.animName && it->controlIdx == ar.controlIdx) {
				animationReel.erase(it);
				break;
			}
		}
		initResetWidthAnim(pid);
		animationReel.push_back(ar);
	}

	void NVGView::controlEndEdit(CControl* pControl) {
		/*
		int pid = pControl->getTag();
		int idx = WIDTH_RESET - pid;
		controlMap[resetSwitch[idx]]->setValueNormalized(0.5f);
		Anima ar;
		ar.animName = BUTTON_BLINK_;
		ar.controlIdx = idx;
		ar.animState = 0;
		animationReel.push_back(ar);
		*/
	}

	void NVGView::setupRCButton() {
		CBButton* cb = dynamic_cast<CBButton*> ((*viewListIter).get());
		
		cButtonMap.emplace((*viewListIter).get()->customRSViewName, cb);
		controlMap[(*cButtonMap[(*viewListIter).get()->customRSViewName]).getTag()] = cb;
		//cb->setListener(this);
		//stupid  listner doesnt respond to controller!
		//cb->getTag();GET THE TAG?
		//controlsList[tag]
		//we dont need another functionList[gprams]
		//we need controlsList[gparams] here...
		//or controlsList[cbuttonNames[]]
		RSBView newBView((*viewListIter).get()->customRSViewName);
		

		newBView.type = (*viewListIter).get()->customRSType;//TEST
		newBView.value = (*cButtonMap[(*viewListIter).get()->customRSViewName]).getValue();
		newBView.drawRect = CRect((*viewListIter).get()->getViewSize());
		newBView.baseX = (*viewListIter).get()->getViewSize().getTopLeft().x;
		newBView.baseY = (*viewListIter).get()->getViewSize().getTopLeft().y;
		newBView.baseZ = 0;

		Draw.createRData((*viewListIter).get()->customRSViewName, newBView);
		Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawCButton);
		RDrawVector[1].push_back((*viewListIter).get()->customRSViewName);
		RViewsMap.emplace((*viewListIter).get()->customRSViewName, newBView);
		viewIndex[(*cButtonMap[(*viewListIter).get()->customRSViewName]).getTag()] = &RViewsMap.at((*viewListIter).get()->customRSViewName );
		viewsReady[(*cButtonMap[(*viewListIter).get()->customRSViewName]).getTag()] = true;
		setupDisplayActive((*viewListIter).get());

		//EXAMPLE OF LATE BINDING GUI EVENTS 
		//requires cbbutton to be reventable
		/*
		if ((*viewListIter).get()->customRSViewName == "width1Reset") {
			cb->fMap.updateContext("onMouseUp", this);
			cb->fMap.reBind("onMouseUp", this, &RDataConsumer::onRCButtonUp);
		}
		*/

	}


	void NVGView::setupRKnob() {
		
		CBKnob* cbk = dynamic_cast<CBKnob*> ((*viewListIter).get());
		knobMap.emplace((*viewListIter).get()->customRSViewName, cbk);
		controlMap[(*knobMap[(*viewListIter).get()->customRSViewName]).getTag()] = cbk;
		//configure the cbknob view
		RSBView newBView((*viewListIter).get()->customRSViewName);
		newBView.type = 1; //(*viewListIter).get()->customRSType;
		newBView.value = (*knobMap[(*viewListIter).get()->customRSViewName]).getValue();
		newBView.drawRect = CRect( (*viewListIter).get()->getViewSize() );
		newBView.baseX = (*viewListIter).get()->getViewSize().getTopLeft().x;
		newBView.baseY = (*viewListIter).get()->getViewSize().getTopLeft().y;
		newBView.baseZ = 0;
		newBView.alpha = 80.0f;
		//cbk->valueToPoint(newBView.loc);
		//set the callback functions for drawing
		Draw.createRData((*viewListIter).get()->customRSViewName, newBView);
		Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawKnob);
		//map another customRSType to setup another style of knob and bind to a different draing function to draw diffrerently
		RDrawVector[1].push_back((*viewListIter).get()->customRSViewName);

		setupDisplayActive((*viewListIter).get());

		if (cbk->getBackground() == NULL) {
			
		}else {
			if ((*viewListIter).get()->customRSViewName == "decay") {
				setupPNG(viewListIter, KNOB_BKG1);
			}else if ((*viewListIter).get()->customRSViewName == "modGain") {
				setupPNG(viewListIter, KNOB2_BKG1);
			}
			
		}

		if (pngType == KNOB_BKG1) {
			newBView.viewType = KNOB_BKG1;
			newBView.strokeWidth1 = 2.0f;
		}
		else if (pngType == KNOB2_BKG1) {
			newBView.viewType = KNOB2_BKG1;
			newBView.strokeWidth1 = 3.0f;

		}
		/* tailor based on custmoRSViewNamechange based on */
		if ((*viewListIter).get()->customRSViewName == "attack" || (*viewListIter).get()->customRSViewName == "attack_2") {
			newBView.overlayType = ATTACK_OVERLAY;
			newBView.overlayWidth = 18;
			newBView.overlayHeight = 18;
		}else if ((*viewListIter).get()->customRSViewName == "decay" || (*viewListIter).get()->customRSViewName == "decay_2") {
			newBView.overlayType = DECAY_OVERLAY;
			newBView.overlayWidth = 18;
			newBView.overlayHeight = 18;
		}
		else if ((*viewListIter).get()->customRSViewName == "release" || (*viewListIter).get()->customRSViewName == "release_2") {
			newBView.overlayType = RELEASE_OVERLAY;
			newBView.overlayWidth = 18;
			newBView.overlayHeight = 18;
		}
		else {
			newBView.overlayType = NONE;
		}

		RViewsMap.emplace((*viewListIter).get()->customRSViewName, newBView);
		viewIndex[(*knobMap[(*viewListIter).get()->customRSViewName]).getTag()] = &RViewsMap.at((*viewListIter).get()->customRSViewName);
		viewsReady[(*knobMap[(*viewListIter).get()->customRSViewName]).getTag()] = true;

	}

	void NVGView::setupRKnob2() {

		CBKnob* cbk = dynamic_cast<CBKnob*> ((*viewListIter).get());
		knobMap.emplace((*viewListIter).get()->customRSViewName, cbk);
		controlMap[(*knobMap[(*viewListIter).get()->customRSViewName]).getTag()] = cbk;
		//configure the cbknob view
		RSBView newBView((*viewListIter).get()->customRSViewName);
		newBView.type = 1; //(*viewListIter).get()->customRSType;
		newBView.value = (*knobMap[(*viewListIter).get()->customRSViewName]).getValue();
		newBView.drawRect = CRect((*viewListIter).get()->getViewSize());
		newBView.baseX = (*viewListIter).get()->getViewSize().getTopLeft().x;
		newBView.baseY = (*viewListIter).get()->getViewSize().getTopLeft().y;
		newBView.baseZ = 0;
		newBView.alpha = 80.0f;
		//cbk->valueToPoint(newBView.loc);
		//set the callback functions for drawing
		Draw.createRData((*viewListIter).get()->customRSViewName, newBView);
		Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawKnob2);
		//map another customRSType to setup another style of knob and bind to a different draing function to draw diffrerently
		RDrawVector[1].push_back((*viewListIter).get()->customRSViewName);

		setupDisplayActive((*viewListIter).get());

		RViewsMap.emplace((*viewListIter).get()->customRSViewName, newBView);
		viewIndex[(*knobMap[(*viewListIter).get()->customRSViewName]).getTag()] = &RViewsMap.at((*viewListIter).get()->customRSViewName);
		viewsReady[(*knobMap[(*viewListIter).get()->customRSViewName]).getTag()] = true;

	}


	void NVGView::setupRSlider() {
		
		CBSlider* cbk = dynamic_cast<CBSlider*> ((*viewListIter).get());
		sliderMap.emplace((*viewListIter).get()->customRSViewName, cbk);
		cbk->setWheelInc(0.25f);
		
		//configure the cbknob view
		RSBView newBView((*viewListIter).get()->customRSViewName);
		newBView.type = (*viewListIter).get()->customRSType;
		newBView.value = (*sliderMap[(*viewListIter).get()->customRSViewName]).getValue();
		newBView.drawRect = CRect((*viewListIter).get()->getViewSize());
		newBView.baseX = (*viewListIter).get()->getViewSize().getTopLeft().x;
		newBView.baseY = (*viewListIter).get()->getViewSize().getTopLeft().y;
		newBView.baseZ = 0;


		Draw.createRData((*viewListIter).get()->customRSViewName, newBView);
		Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawSlider);
		RDrawVector[1].push_back((*viewListIter).get()->customRSViewName);
		RViewsMap.emplace((*viewListIter).get()->customRSViewName, newBView);

		setupDisplayActive((*viewListIter).get());
		

	}

	void NVGView::setupRSliderVert() {
		
		CBSlider* cbk = dynamic_cast<CBSlider*> ((*viewListIter).get());
		sliderMap.emplace((*viewListIter).get()->customRSViewName, cbk);
		cbk->setWheelInc(0.1f);

		//configure the cbknob view
		RSBView newBView((*viewListIter).get()->customRSViewName);
		newBView.type = (*viewListIter).get()->customRSType;
		newBView.value = (*sliderMap[(*viewListIter).get()->customRSViewName]).getValue();
		newBView.drawRect = CRect((*viewListIter).get()->getViewSize());
		newBView.baseX = (*viewListIter).get()->getViewSize().getTopLeft().x;
		newBView.baseY = (*viewListIter).get()->getViewSize().getTopLeft().y;
		newBView.baseZ = 0;

		Draw.createRData((*viewListIter).get()->customRSViewName, newBView);
		Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawSliderVert);
		RDrawVector[1].push_back((*viewListIter).get()->customRSViewName);
		RViewsMap.emplace((*viewListIter).get()->customRSViewName, newBView);
		
		setupDisplayActive((*viewListIter).get());
		
	}

	
	void NVGView::setupRSwitchVert() {
		CBVerticalSwitch* cbk = dynamic_cast<CBVerticalSwitch*> ((*viewListIter).get());
		//VSwitchListener* cbkListener = new VSwitchListener();
		//(*cbk).registerControlListener(cbkListener);
		
		vertSwitchMap.emplace((*viewListIter).get()->customRSViewName, cbk);

		controlMap[(*vertSwitchMap[(*viewListIter).get()->customRSViewName]).getTag()] = cbk;

		RSBView newBView((*viewListIter).get()->customRSViewName);
		newBView.type = (*viewListIter).get()->customRSType;
		newBView.value = (*vertSwitchMap[(*viewListIter).get()->customRSViewName]).getValue();
		newBView.drawRect = CRect((*viewListIter).get()->getViewSize());
		newBView.baseX = (*viewListIter).get()->getViewSize().getTopLeft().x;
		newBView.baseY = (*viewListIter).get()->getViewSize().getTopLeft().y;
		newBView.baseZ = 0;
		newBView.alpha = 80.0f;
		newBView.height = (newBView.drawRect.getHeight() / (*vertSwitchMap[(*viewListIter).get()->customRSViewName]).getMax()) - ((newBView.drawRect.getHeight() / cbk->getMax()) / cbk->getMax());
		Draw.createRData((*viewListIter).get()->customRSViewName, newBView);
		Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawSwitchVert);
		RDrawVector[1].push_back((*viewListIter).get()->customRSViewName);
		RViewsMap.emplace((*viewListIter).get()->customRSViewName, newBView);

		viewIndex[(*vertSwitchMap[(*viewListIter).get()->customRSViewName]).getTag()] = &RViewsMap.at((*viewListIter).get()->customRSViewName);
		viewsReady[(*vertSwitchMap[(*viewListIter).get()->customRSViewName]).getTag()] = true;

		setupDisplayActive((*viewListIter).get());


	}

	void NVGView::setupStepCtrlBtn() {
		CBStepCtrlButton* ctb = dynamic_cast<CBStepCtrlButton*> ((*viewListIter).get());

		
		/*
		ctb->setCallback([ctb, this]() {
			int tag = ctb->getTag();
			float value = ctb->getValue();
			this->baseController->setParamNormalized(tag, value);
		});
		*/
		//MKAE cTextButtonMap
		stepCtrlButtonMap.emplace((*viewListIter).get()->customRSViewName, ctb);
	
		controlMap[(*stepCtrlButtonMap[(*viewListIter).get()->customRSViewName]).getTag()] = ctb;

		RSBView newBView((*viewListIter).get()->customRSViewName);
		newBView.type = (*viewListIter).get()->customRSType;
		newBView.value = (*stepCtrlButtonMap[(*viewListIter).get()->customRSViewName]).getValue();
		newBView.drawRect = CRect((*viewListIter).get()->getViewSize());
		newBView.baseX = (*viewListIter).get()->getViewSize().getTopRight().x;
		newBView.baseY = (*viewListIter).get()->getViewSize().getTopRight().y;
		newBView.baseZ = 0;
		newBView.height = newBView.drawRect.getHeight();
		newBView.width = newBView.drawRect.getWidth();
		newBView.lowerRightX = newBView.baseX;
		newBView.lowerRightY = newBView.baseY + newBView.height;
		newBView.lowerLeftX = newBView.baseX - newBView.width;
		newBView.lowerLeftY = newBView.lowerRightY;
		newBView.upperLeftX1 = newBView.baseX - newBView.width;
		newBView.upperLeftY1 = newBView.baseY + 6.0f;
		newBView.upperLeftX2 = newBView.baseX - (newBView.width -6.0f);
		newBView.upperLeftY2 = newBView.baseY;
		newBView.alpha = 80.0f;
		Draw.createRData((*viewListIter).get()->customRSViewName, newBView);
		Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawStepCtrlBtn);
		RDrawVector[1].push_back((*viewListIter).get()->customRSViewName);
		RViewsMap.emplace((*viewListIter).get()->customRSViewName, newBView);

		viewIndex[(*stepCtrlButtonMap[(*viewListIter).get()->customRSViewName]).getTag()] = &RViewsMap.at((*viewListIter).get()->customRSViewName);

		viewsReady[(*stepCtrlButtonMap[(*viewListIter).get()->customRSViewName]).getTag()] = true;

		setupDisplayActive((*viewListIter).get());
		
		
	}

	void NVGView::setupSineSymbol() {
		//get the object
		CView* cbk = dynamic_cast<CView*> ((*viewListIter).get());
		viewMap.emplace((*viewListIter).get()->customRSViewName, cbk);
		RSBView newBView((*viewListIter).get()->customRSViewName);
		newBView.type = (*viewListIter).get()->customRSType;
		newBView.drawRect = CRect((*viewListIter).get()->getViewSize());
		newBView.baseX = (*viewListIter).get()->getViewSize().getTopLeft().x;
		newBView.baseY = (*viewListIter).get()->getViewSize().getTopLeft().y;
		newBView.baseZ = 0;
		Draw.createRData((*viewListIter).get()->customRSViewName, newBView);
		if (strcmp(oscType.c_str(), "sine") == 0) {
			newBView.viewType = SINE_ICON;
			Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawSineSymbol);
		}
		if (strcmp(oscType.c_str(), "saw") == 0) {
			newBView.viewType = SAW_ICON;
			Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawSineSymbol);
		}
		if (strcmp(oscType.c_str(), "tri") == 0) {
			newBView.viewType = TRI_ICON;
			Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawSineSymbol);
		}
		if (strcmp(oscType.c_str(), "square") == 0) {
			newBView.viewType = SQUARE_ICON;
			Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawSineSymbol);
		}

		RDrawVector[1].push_back((*viewListIter).get()->customRSViewName);
		RViewsMap.emplace((*viewListIter).get()->customRSViewName, newBView);
		setupDisplayActive((*viewListIter).get());
		
		//get the image from VSTGUI and give it to NVG:>
		/*
		SharedPointer<IPlatformBitmap> cbit;
		//PNGBitmapBuffer cbuf = IPlatformBitmap::createMemoryPNGRepresentation((*viewListIter).get()->getBackground()->getPlatformBitmap());
		cbuf = IPlatformBitmap::createMemoryPNGRepresentation((*viewListIter).get()->getBackground()->getPlatformBitmap());
		//const size_t charSize = cbuf.size();
		charBuffer = new unsigned char[cbuf.size()];
		//charBuffer = char[cbuf.size()];

		//charBuffy = reinterpret_cast<char*>(cbuf.data());
		for (size_t i = 0; i < cbuf.size(); i++) {
			charBuffer[i] = (unsigned char)cbuf[i];
		}
		images[newBView.viewType] = nvgCreateImageMem(vg, 0, charBuffer, (int)cbuf.size());
		*/


	}



	void NVGView::setupImage() {

		if ((*viewListIter).get()->customRSViewName == "attack_overlay") {
			setupPNG(viewListIter, ATTACK_OVERLAY);
		}
		else if ((*viewListIter).get()->customRSViewName == "decay_overlay") {
			setupPNG(viewListIter, DECAY_OVERLAY);
		}
		else if ((*viewListIter).get()->customRSViewName == "release_overlay") {
			setupPNG(viewListIter, RELEASE_OVERLAY);
		}
		else if ((*viewListIter).get()->customRSViewName == "mainRowBkg") {
			setupPNG(viewListIter, MAIN_ROW_BKG);
		}
		else if ((*viewListIter).get()->customRSViewName == "dmAboutWindow") {
			setupPNG(viewListIter, DM_ABOUT_WINDOW);
		}

	}

	void NVGView::setupMoreInfoBtn() {
			moreInfoBtn = dynamic_cast<CBMoreInfoButton*> ((*viewListIter).get());
			moreInfoBtn->onMouseEnterCallback = std::bind(&NVGView::onMoreInfoBtnEnter, this);
			moreInfoBtn->onMouseExitCallback = std::bind(&NVGView::onMoreInfoBtnExit, this);
			moreInfoBtn->onMouseDownCallback = std::bind(&NVGView::onMoreInfoBtnDown, this);
			RSBView newBView((*viewListIter).get()->customRSViewName);
			newBView.type = 1; //(*viewListIter).get()->customRSType;
			newBView.drawRect = CRect((*viewListIter).get()->getViewSize());
			newBView.baseX = (*viewListIter).get()->getViewSize().getTopLeft().x;
			newBView.baseY = (*viewListIter).get()->getViewSize().getTopLeft().y;
			newBView.baseZ = 0;
			newBView.alpha = 80.0f;
			Draw.createRData((*viewListIter).get()->customRSViewName, newBView);
			Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawMoreInfoBtn);
			RDrawVector[1].push_back((*viewListIter).get()->customRSViewName);
			setupDisplayActive((*viewListIter).get());
			RViewsMap.emplace("MORE_INFO_BTN", newBView);
			viewIndex[MORE_INFO_BUTTON_VIEW_IDX] = &RViewsMap.at("MORE_INFO_BTN");
	

	}

	void NVGView::setupShutWinBtn() {
		CBButton* cbtn = dynamic_cast<CBButton*> ((*viewListIter).get());
		cbtn->onMouseEnterCallback = std::bind(&NVGView::onMoreInfoBtnEnter, this);
		cbtn->onMouseExitCallback = std::bind(&NVGView::onMoreInfoBtnExit, this);
		cbtn->onMouseDownCallback = std::bind(&NVGView::onShutWinBtnDown, this);
		cbtn->setMouseEnabled(false);
		RSBView newBView((*viewListIter).get()->customRSViewName);
		newBView.type = 1; //(*viewListIter).get()->customRSType;
		newBView.drawRect = CRect((*viewListIter).get()->getViewSize());
		newBView.baseX = (*viewListIter).get()->getViewSize().getTopLeft().x;
		newBView.baseY = (*viewListIter).get()->getViewSize().getTopLeft().y;
		newBView.baseZ = 0;
		newBView.alpha = 80.0f;
		//Draw.createRData((*viewListIter).get()->customRSViewName, newBView);
		//Draw.createRDataBinder((*viewListIter).get()->customRSViewName, (*viewListIter).get()->customRSViewName, this, &RDataConsumer::drawMoreInfoBtn);
		//RDrawVector[1].push_back((*viewListIter).get()->customRSViewName);
		//setupDisplayActive((*viewListIter).get());
		controlMap[SHUT_WIN_BUTTON_VIEW_IDX] = cbtn;
		RViewsMap.emplace("SHUT_WIN_BTN", newBView);
		viewIndex[SHUT_WIN_BUTTON_VIEW_IDX] = &RViewsMap.at("SHUT_WIN_BTN");
		

	}


	void NVGView::setupPNG(std::list<SharedPointer<CView> >::const_iterator theView, int intKey ) {

		if ( images[intKey] < 1 ) {
			//SharedPointer<IPlatformBitmap> cbit;
			cbuf = IPlatformBitmap::createMemoryPNGRepresentation((*theView).get()->getBackground()->getPlatformBitmap());
			const size_t charSize = cbuf.size();
			
			//charBuffer = new unsigned char[charSize];
			for (size_t i = 0; i < cbuf.size(); i++) {
				charBuffer[i] = (unsigned char)cbuf[i];
			}

			images[intKey] = nvgCreateImageMem(vg, 0, charBuffer, (int)charSize);
		}
		
	}

	void NVGView::resetBKG() {
		const size_t charSize = cbuf.size();
		//charBuffer = new unsigned char[charSize];
		for (size_t i = 0; i < cbuf.size(); i++) {
			charBuffer[i] = (unsigned char)cbuf[i];
		}
		images[0] = nvgCreateImageMem(vg, 0, charBuffer, (int)charSize);
	}

	void NVGView::setupDisplayActive(CView* cvp) {
		if (cvp->getTransparency() == true) {
			cvp->customRSDisplayActive = false;
			cvp->setMouseEnabled(false);
			Draw.boundHandlers.at(cvp->customRSViewName).boundHandler = &RDataConsumer::drawBlank;
		}else {
			cvp->customRSDisplayActive = true;
			cvp->setMouseEnabled(true);
		}
	}

	void NVGView::setDisplayActive(CView* cvp, void (RDataConsumer::*handler)(RData& e)) {
		cvp->customRSDisplayActive = true;
		cvp->setTransparency(false);
		cvp->setMouseEnabled(true);
		Draw.boundHandlers.at(cvp->customRSViewName).boundHandler = handler;
		
	}

	void NVGView::setDisplayInactive(CView* cvp) {
		cvp->setTransparency(true);
		cvp->customRSDisplayActive = false;
		cvp->setMouseEnabled(false);
		Draw.boundHandlers.at(cvp->customRSViewName).boundHandler = &RDataConsumer::drawBlank;
	}

	void NVGView::toggleDisplayActive(CView* cvp, void (RDataConsumer::*handler)(RData& e) ) {
		if (cvp->getTransparency() == false) {
			cvp->setTransparency(true);
			cvp->customRSDisplayActive = false;
			cvp->setMouseEnabled(false);
			Draw.boundHandlers.at(cvp->customRSViewName).boundHandler = &RDataConsumer::drawBlank;
		}else {
			cvp->customRSDisplayActive = true;
			cvp->setTransparency(false);
			cvp->setMouseEnabled(true);
			Draw.boundHandlers.at(cvp->customRSViewName).boundHandler = handler;

		}
	}

	/*
	int32_t NVGView::onKeyDown(VstKeyCode &  keyCode) {

		int ainint = 0;
		int anInt2 = 2;
	}
	*/

	bool NVGView::attached(CView* parent) {
	if (COpenGLView::attached(parent)) {

		

		for (int i = 0; i < 1000; i++) {
			controlMap[i] = nullptr;
		}

		vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
		fontInt = nvgCreateFontMem(vg, "lucon", lucon_ttf, lucon_ttf_len, 1);
		
		nvgFontSize(vg, 9.0f);

		// Sets the blur of current text style.
		nvgFontBlur(vg, 0.0f);


		RSBView background("bkg");
		background.type = 0;
		background.value = 100.0;
		Draw.createRData("bkg", background);
		Draw.createRDataBinder("background", "bkg", this, &RDataConsumer::drawBkg);
		

		RViewsMap.emplace("bkg", background);
		RDrawVector[0].push_back("background");

		
		//oscScopeX = 10.0;
		//oscScopeY = 10.0;
		//oscScopeH = 180.0;
		//oscScopeW = 180.0;

		if (getEditor()->getFrame()) {

			//getAllViews method added to cviewcontainer
			getFrame()->getViewsAll(views);
			control = nullptr;//do we need this
			viewListIter = views.begin();

			while (viewListIter != views.end()) {
				// types set in uidescription files custom-view-name and sub-controller fields and used to set cview properties customRSViewName and customRSType in modded cview 
				if ((*viewListIter).get()->customRSType > -1) {// custom type is defined, get customName and configure

					if ((*viewListIter).get()->customRSType == 1) { // type 1 : cbknob // can we rfunction this?
						if ((*viewListIter).get()->customRSViewName.empty() == false) {//gotta have a name here
							pngType = KNOB_BKG1;
							fMap.callFunction("setupRKnob");
						}
					}
					
					else if ((*viewListIter).get()->customRSType == 2) { //  cbbutton
						if ((*viewListIter).get()->customRSViewName.empty() == false) {//gotta have a name here
							fMap.callFunction("setupRButton");
						}
					}

					else if ((*viewListIter).get()->customRSType == 3) { // cbslider horizontal 
						if ((*viewListIter).get()->customRSViewName.empty() == false) {//gotta have a name here
							fMap.callFunction("setupRSlider");
						}
					}
					else if ((*viewListIter).get()->customRSType == 4) { // cbslider vert 
						if ((*viewListIter).get()->customRSViewName.empty() == false) {//gotta have a name here
							fMap.callFunction("setupRSwitchVert");
						}
					}else if ((*viewListIter).get()->customRSType == 5) { // cbslider vert 
						if ((*viewListIter).get()->customRSViewName.empty() == false) {//gotta have a name here
							fMap.callFunction("setupRSliderVert");
						}
					}else if ((*viewListIter).get()->customRSType == 6) { // cbslider vert 
						if ((*viewListIter).get()->customRSViewName.empty() == false) {//gotta have a name here
							
							if (strcmp( (*viewListIter).get()->customRSViewName.c_str(), "sineSymbol_1") == 0 
							|| strcmp((*viewListIter).get()->customRSViewName.c_str(), "sineSymbol_2") == 0
							|| strcmp((*viewListIter).get()->customRSViewName.c_str(), "sineSymbol_3") == 0
							|| strcmp((*viewListIter).get()->customRSViewName.c_str(), "sineSymbol_4") == 0
							){
								oscType = "sine";
								fMap.callFunction("setupSineSymbol");
							}else if (strcmp((*viewListIter).get()->customRSViewName.c_str(), "sawSymbol_1") == 0
							|| strcmp((*viewListIter).get()->customRSViewName.c_str(), "sawSymbol_2") == 0
							|| strcmp((*viewListIter).get()->customRSViewName.c_str(), "sawSymbol_3") == 0
							|| strcmp((*viewListIter).get()->customRSViewName.c_str(), "sawSymbol_4") == 0
							) {
								oscType = "saw";
								fMap.callFunction("setupSineSymbol");		
							}else if (strcmp((*viewListIter).get()->customRSViewName.c_str(), "triSymbol_1") == 0
								|| strcmp((*viewListIter).get()->customRSViewName.c_str(), "triSymbol_2") == 0
								|| strcmp((*viewListIter).get()->customRSViewName.c_str(), "triSymbol_3") == 0
								|| strcmp((*viewListIter).get()->customRSViewName.c_str(), "triSymbol_4") == 0
								) {
								oscType = "tri";
								fMap.callFunction("setupSineSymbol");
							}else if (strcmp((*viewListIter).get()->customRSViewName.c_str(), "squareSymbol_1") == 0
								|| strcmp((*viewListIter).get()->customRSViewName.c_str(), "squareSymbol_2") == 0
								|| strcmp((*viewListIter).get()->customRSViewName.c_str(), "squareSymbol_3") == 0
								|| strcmp((*viewListIter).get()->customRSViewName.c_str(), "squareSymbol_4") == 0
								) {
								oscType = "square";
								fMap.callFunction("setupSineSymbol");
							}
							
						}
					}else if ((*viewListIter).get()->customRSType == 7) { 
						if ((*viewListIter).get()->customRSViewName.empty() == false) {
							pngType = KNOB2_BKG1;
							fMap.callFunction("setupRKnob");
						}
					}else if ((*viewListIter).get()->customRSType == 8) { 
						if ((*viewListIter).get()->customRSViewName.empty() == false) {

							fMap.callFunction("setupImage");

						}
					}else if ((*viewListIter).get()->customRSType == 9) {
						if ((*viewListIter).get()->customRSViewName.empty() == false) {

							fMap.callFunction("setupRClickButton");

						}
					}
					else if ((*viewListIter).get()->customRSType == 10) { // cbslider vert 
						if ((*viewListIter).get()->customRSViewName.empty() == false) {//gotta have a name here
							fMap.callFunction("setupRSlider");
						}

					}else if ((*viewListIter).get()->customRSType == 11) { // cbslider vert 
						if ((*viewListIter).get()->customRSViewName.empty() == false) {//gotta have a name here
							fMap.callFunction("setupStepCtrlBtn");
						}

					}
					if ((*viewListIter).get()->customRSType == 12) { // type knob 2
						if ((*viewListIter).get()->customRSViewName.empty() == false) {//gotta have a name here
							fMap.callFunction("setupRKnob2");
						}
					}
					if ((*viewListIter).get()->customRSType == 13) { // more info button
						if ((*viewListIter).get()->customRSViewName.empty() == false) {//gotta have a name here
							setupMoreInfoBtn();
						}
					}
					if ((*viewListIter).get()->customRSType == 14) { // shut win btn
						if ((*viewListIter).get()->customRSViewName.empty() == false) {//gotta have a name here
							setupShutWinBtn();
						}
					}

				}
				
						
				viewListIter++;
			}

		}

		//SYNC UP VIEWS WITH PARAMS ON REOPEN HERE
		//



		selectedStepTarg = viewIndex[OCT_];
		selectedStepTarg->RSBType = 1;

		if (baseController) {
			baseController->setViewsFromBase();
		}
		

		/*if (baseController->regKeyLegit == false) {
			onBadReg(0, 0.0);
		}*/

		return true;
	}
	else { 
		return false;
	}

}


	int NVGView::runNVGView(int id) {
		//SFC.drawScreen(xRotation);
		return 0;
	}


	void NVGView::killThread() {
		
		/*if (thread)
		{
			thread->stop ();
			thread->waitDead (-1);
			delete thread;
			thread = 0;
		}*/
		
	}

	//REOME THIS FOR EDITOR
	bool NVGView::removed(CView* parent) {
		shouldDraw = false;

		baseController->nvgViewReady = false;

		views.clear();
		cbuf.clear();
		/*
		animationReel.clear();

		RDrawViews.clear();
		RDrawVector->clear();
		RDrawVectorOne.clear();
		RViewsMap.clear();
		viewTypes.clear();
		views.clear();
		viewMap.clear();
		setupViewsMap.clear();
		knobMap.clear();
		buttonMap.clear();
		cButtonMap.clear();
		sliderMap.clear();
		vertSwitchMap.clear();
		stepCtrlButtonMap.clear();
		viewMapKeys.clear();
		*/

		removeAnimation("XRotation");
		for (int i = 0; i < 3; i++) {
			RDrawVector[i].clear();
		}

		if (vg) {
			nvgDeleteGL3(vg);
			vg = nullptr;

		}

		COpenGLView::removed(parent);

		return true;
	}
	

	void NVGView::setThreaded(bool state){
		if (isAttached()){
			xRotation = yRotation = zRotation = 0.f;
			if (state == true){
				//thread = new Thread(this);
				//thread->run();
				//thread = new sf::Thread(drawOpenGLThreaded);

			}
			if (state == false){
				//draw loop dependent on this
				killThread();
				remember();
				addAnimation("XRotation", this, new Animation::RepeatTimingFunction(new Animation::LinearTimingFunction(4000), -1, false));
				
			}
		}
		useThread = state;
	}

	void NVGView::platformOpenGLViewCreated() {
			
		getPlatformOpenGLView()->lockContext();
		getPlatformOpenGLView()->makeContextCurrent();
		
		glewInit();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glEnable(GL_STENCIL_TEST);
		glEnable(GL_MULTISAMPLE);
		//glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		//glStencilFunc(GL_ALWAYS, 1, 0xFF); // all fragments should pass the stencil test
		//glStencilMask(0xFF);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
		glClearColor(0, 0, 0, 0);

		glGetIntegerv(GL_SAMPLE_BUFFERS, &bufs);
		glGetIntegerv(GL_SAMPLES, &samples);
		
		getPlatformOpenGLView()->unlockContext();
		setThreaded(false);
		bool isCreated = true;
		isCreatedPtr = &isCreated;
	}

	void NVGView::platformOpenGLViewWillDestroy() {

		killThread();
		removeAllAnimations();
	}

	void NVGView::platformOpenGLViewSizeChanged() {

		if (getPlatformOpenGLView())
		{
			getPlatformOpenGLView()->lockContext();
			getPlatformOpenGLView()->makeContextCurrent();

			CRect r(getViewSize());
				
			glViewport(0, 0, static_cast<GLsizei> (r.getWidth()), static_cast<GLsizei> (r.getHeight()));

			getPlatformOpenGLView()->unlockContext();
		}
	}

	void NVGView::drawOpenGLThreaded() {
		if (getPlatformOpenGLView())
		{
				
			
		}
	}






	//NOT USED
	void NVGView::animationTick(CView* view, IdStringPtr name, float pos) {
		float p = pos;
		if (strcmp(name, "blinkButton") == 0) {
			//float p = pos;
			//viewIndex[view.]->blinkerDone = false;
			if (RViewsMap.at("width1Reset").blinker == 0.0 && RViewsMap.at("width1Reset").blinkerDone == false) {
				RViewsMap.at("width1Reset").blinkIntensity += 0.033;
				if (RViewsMap.at("width1Reset").blinkIntensity >= 1.0) {
					RViewsMap.at("width1Reset").blinkIntensity = 1.0;
					RViewsMap.at("width1Reset").blinker = 1.0;
				}
			}
			else if(RViewsMap.at("width1Reset").blinker == 1.0 && RViewsMap.at("width1Reset").blinkerDone == false){
				RViewsMap.at("width1Reset").blinkIntensity -= 0.033;
				if (RViewsMap.at("width1Reset").blinkIntensity <= 0.0) {
					RViewsMap.at("width1Reset").blinkIntensity = 0.0;
					RViewsMap.at("width1Reset").blinker = 0.0;
					RViewsMap.at("width1Reset").blinkerDone = true;
				}
			}
			
			
		}
		else if (strcmp(name, "hideGen2") == 0) {
			
			
		}

		invalid();
	}

	void NVGView::animationStart(CView* view, IdStringPtr name) {
		if (strcmp(name, "blinkButton") == 0) {
			RViewsMap.at("width1Reset").blinker = 0.0;
			RViewsMap.at("width1Reset").blinkerDone = false;
		}
		else if (strcmp(name, "hideGen2") == 0) {

		}


	};


	void NVGView::animationFinished(CView* view, IdStringPtr name, bool wasCanceled) {
	

	}
	

	int NVGView::buttonBlink(RSBView* rv) {
		if (!rv) {
			return 0;
		}else{
			if (rv->blinkerDone == true) {
				return 1;
			}
			else {
				if (rv->blinker == 0.0 && rv->blinkerDone == false) {
					rv->blinkIntensity += 0.033;
					if (rv->blinkIntensity >= 1.0) {
						rv->blinkIntensity = 1.0;
						rv->blinker = 1.0;
					}
				}
				else if (rv->blinker == 1.0 && rv->blinkerDone == false) {
					rv->blinkIntensity -= 0.033;
					if (rv->blinkIntensity <= 0.003) {
						rv->blinkIntensity = 0.0;
						rv->blinker = 0.0;
						rv->blinkerDone = true;
					}
				}
				return 0;
			}
		}

	}

	int NVGView::stepTargBlink(RSBView* rv) {
		if (rv->blinkerDone == true) {
			return 1;
		}
		else {
			if (rv->blinker == 0.0 && rv->blinkerDone == false) {
				rv->blinkIntensity += 0.066;
				if (rv->blinkIntensity >= 1.0) {
					rv->blinkIntensity = 1.0;
					rv->blinker = 1.0;
				}
			}
			else if (rv->blinker == 1.0 && rv->blinkerDone == false) {
				rv->blinkIntensity -= 0.033;
				if (rv->blinkIntensity <= 0.003) {
					rv->blinkIntensity = 0.0;
					rv->blinker = 0.0;
					rv->blinkerDone = true;
				}
			}
			return 0;
		}
	}

	int NVGView::buttonGlow(RSBView* rv) {
		rv->blinkIntensity = buttonMap[rv->viewName]->getValue();
		if (rv->blinker == rv->blinkIntensity) {
			return 1;
		}
		if (rv->blinkIntensity > 0.0) {
			if (rv->blinker < rv->blinkIntensity) {
				rv->blinker += 0.05;
				if (rv->blinker > 1.0) {
					rv->blinker = 1.0;
				}
			}
		}else if (rv->blinkIntensity  == 0.0 ) {
			if (rv->blinker > rv->blinkIntensity) {
				rv->blinker -= 0.05;
				if (rv->blinker < 0.0) {
					rv->blinker = 0.0;
				}
			}
		}
		return 0;
	}

	//DRAWING LOOP
	void NVGView::drawOpenGL(const CRect& updateRect) {
		
		if (this->baseController) {
			if ( this->baseController->nvgViewReady == true ){
				this->baseController->onNVGViewReady();
			}
		}
		//if (shouldDraw == true) {



			for (int i = 0; i < animationReel.size(); i++) {

				if (animationReel[i].animState == 0) {//if index 2 (is playing) is zero, play, but set is playing to one from animation function if ended.				
					animationReel[i].animState = (this->*nvgAnimations[animationReel[i].animName])(viewIndex[animationReel[i].controlIdx]);

				}if (animationReel[i].animState == 1) {
					animationReel.erase(animationReel.begin() + i);
				}
			}
			if (stepCtrlSelectAnim.animState == 0) {
				(this->*nvgAnimations[stepCtrlSelectAnim.animName])(viewIndex[stepCtrlSelectAnim.controlIdx]);
			}

			glClearColor(0, 0, 0, 0);
			glStencilMask(0xff);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			nvgBeginFrame(vg, 1148, 542, 1);

			//dravw envs
			nvgLineJoin(vg, NVG_ROUND);
			nvgLineCap(vg, NVG_ROUND);
			drawEnv1();
			drawEnv2();
			drawEnv3();
			// draw lfo
			drawEnv4();
			drawEnv5();
			drawEnv6();

			nvgLineJoin(vg, NVG_MITER);
			nvgLineCap(vg, NVG_BUTT);

			//layer 0
			for (int i = 0; i < RDrawVector[0].size(); i++) {
				Draw.execDataHandler(RDrawVector[0][i]);
			}
			//layer 1

			for (int i = 0; i < RDrawVector[1].size(); i++) {

				Draw.execDataHandler(RDrawVector[1][i]);
			}

			drawSelctedStepTarg();
			drawCtrlValueOutput();
			drawBeatModOn();

			drawMoreInfoWindow();

			//need isRegValid bool here from processor
			if (badReg == true) {
				drawBadReg();
			}

			nvgEndFrame(vg);
			//drawOGL();
			getPlatformOpenGLView()->swapBuffers();


			glFinish();
			invalid();
		//}
	}

	//NOT SHOWING ENOUGH RESOLUTION FOR FIRST TWO BPM SYNC STATES
	void NVGView::drawEnv4() {
		lfo1Amp = knobMap.at("lfo1Amp")->getValueNormalized();
		lfo1Type = int64(knobMap.at("lfo1Type")->getValue());
		//lfo1Rate supplied by message notify from processor
		lfo1Slider = sliderMap.at("lfo1PhaseSlider")->getValue() - 0.5;
		t_lfo1Width = lfoWidth / fmax(0.03, (*lfo1Rate));
		//lfo1OriginX = lfo1OriginStartX + (lfo1Slider * (t_lfo1Width)) - (t_lfo1Width*.25) - (ceil(lfo1Slider) * t_lfo1Width);
		
		nvgBeginPath(vg);
		nvgScissor(vg, 0, 0, 1148, 542);
		nvgIntersectScissor(vg, lfo1OriginStartX, lfo1OriginY - lfoHeight / 2, lfoWidth, lfoHeight);
		nvgMoveTo(vg, lfo1OriginX, lfo1OriginY);
		switch (lfo1Type) {
		case 0:
			lfo1OriginX = lfo1OriginStartX + (lfo1Slider * (t_lfo1Width)) - t_lfo1Width * .5;
			lfo1oX = lfo1OriginX + t_lfo1Width;
			lfo1oY = lfo1OriginY;
			lfo1oX2 = lfo1OriginX;
			lfo1oX2Handle = lfo1oX2 + (t_lfo1Width*.5);
			lfo1Height = lfoMinHeight + lfoHeight * lfo1Amp;
			lfo1Height = lfo1Height * 1.25;
			for (int i = 0; i <= (*lfo1Rate) + 2; i++) {
				lfo1oX2Handle = lfo1oX2 + (t_lfo1Width*.5);
				nvgBezierTo(vg,
					(lfo1oX2Handle),
					(lfo1oY - lfo1Height),
					(lfo1oX2Handle),
					(lfo1oY + lfo1Height),
					lfo1oX, lfo1oY);
				lfo1oX2 = lfo1oX;
				lfo1oX = lfo1oX + t_lfo1Width;
			}
			break;
		case 1:
			lfo1OriginX = lfo1OriginStartX + (lfo1Slider * (t_lfo1Width)) - t_lfo1Width * .5;
			lfo1oX = lfo1OriginX + t_lfo1Width;
			lfo1oY = lfo1OriginY;
			lfo1oX2 = lfo1OriginX;
			lfo1oX2Handle = lfo1oX2 + (t_lfo1Width*.5);
			lfo1Height = lfoMinHeight + lfoHeight * lfo1Amp;
			lfo1Height = lfo1Height * .35;
			for (int i = 0; i <= (*lfo1Rate)+2; i++) {
				nvgLineTo(vg,
					lfo1oX2,
					(lfo1oY - lfo1Height)
				);
				nvgLineTo(vg,
					lfo1oX,
					(lfo1oY + lfo1Height)
				);
				lfo1oX2 = lfo1oX;
				lfo1oX = lfo1oX + t_lfo1Width;
			}
			break;
		case 2:
			lfo1OriginX = lfo1OriginStartX + (lfo1Slider * (t_lfo1Width)) - t_lfo1Width * .5;
			lfo1oX = lfo1OriginX + t_lfo1Width;
			lfo1oY = lfo1OriginY;
			lfo1oX2 = lfo1OriginX;
			lfo1oX2Handle = lfo1oX2 + (t_lfo1Width*.5);
			lfo1Height = lfoMinHeight + lfoHeight * lfo1Amp;
			lfo1Height = lfo1Height * .35;
			nvgMoveTo(vg, lfo1OriginX, lfo1OriginY + lfo1Height);
			for (int i = 0; i <= (*lfo1Rate) + 2; i++) {
				nvgLineTo(vg,
					lfo1oX2+ (t_lfo1Width*.5),
					(lfo1oY - lfo1Height)
				);
				nvgLineTo(vg,
					lfo1oX,
					(lfo1oY + lfo1Height)
				);
				lfo1oX2 = lfo1oX;
				lfo1oX = lfo1oX + t_lfo1Width;
			}
			break;
		case 3:
			lfo1OriginX = lfo1OriginStartX + (lfo1Slider * (t_lfo1Width)) - t_lfo1Width * .5;
			lfo1oX = lfo1OriginX + t_lfo1Width;
			lfo1oY = lfo1OriginY;
			lfo1oX2 = lfo1OriginX;
			lfo1oX2Handle = lfo1oX2 + (t_lfo1Width*.5);
			lfo1Height = lfoMinHeight + lfoHeight * lfo1Amp;
			lfo1Height = lfo1Height * .35;
			nvgMoveTo(vg, lfo1OriginX, lfo1OriginY + lfo1Height);
			for (int i = 0; i <= (*lfo1Rate) + 2; i++) {
				nvgLineTo(vg,
					lfo1oX2,
					(lfo1oY - lfo1Height)
				);
				nvgLineTo(vg,
					lfo1oX2 + (t_lfo1Width*.5),
					(lfo1oY - lfo1Height)
				);
				nvgLineTo(vg,
					lfo1oX2 + (t_lfo1Width*.5),
					(lfo1oY + lfo1Height)
				);
				nvgLineTo(vg,
					lfo1oX,
					(lfo1oY + lfo1Height)
				);
				lfo1oX2 = lfo1oX;
				lfo1oX = lfo1oX + t_lfo1Width;
			}
			break;
		}
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, 2.0F);
		nvgStroke(vg);

		nvgClosePath(vg);
		nvgResetScissor(vg);
			
		
	}

	void NVGView::drawEnv5() {
		lfo2Amp = knobMap.at("lfo2Amp")->getValueNormalized();
		lfo2Type = int64(knobMap.at("lfo2Type")->getValue());
		//lfo2Rate supplied by message notify from processor
		lfo2Slider = sliderMap.at("lfo2PhaseSlider")->getValue() - 0.5;
		t_lfo2Width = lfoWidth / fmax(0.03, (*lfo2Rate));
		//lfo2OriginX = lfo2OriginStartX + (lfo2Slider * (t_lfo2Width)) - (t_lfo2Width*.25) - (ceil(lfo2Slider) * t_lfo2Width);

		nvgBeginPath(vg);
		nvgScissor(vg, 0, 0, 1148, 542);
		nvgIntersectScissor(vg, lfo2OriginStartX, lfo2OriginY - lfoHeight / 2, lfoWidth, lfoHeight);
		nvgMoveTo(vg, lfo2OriginX, lfo2OriginY);
		switch (lfo2Type) {
		case 0:
			lfo2OriginX = lfo2OriginStartX + (lfo2Slider * (t_lfo2Width)) - t_lfo2Width * .5;
			lfo2oX = lfo2OriginX + t_lfo2Width;
			lfo2oY = lfo2OriginY;
			lfo2oX2 = lfo2OriginX;
			lfo2oX2Handle = lfo2oX2 + (t_lfo2Width*.5);
			lfo2Height = lfoMinHeight + lfoHeight * lfo2Amp;
			lfo2Height = lfo2Height * 1.25;
			for (int i = 0; i <= (*lfo2Rate) + 2; i++) {
				lfo2oX2Handle = lfo2oX2 + (t_lfo2Width*.5);
				nvgBezierTo(vg,
					(lfo2oX2Handle),
					(lfo2oY - lfo2Height),
					(lfo2oX2Handle),
					(lfo2oY + lfo2Height),
					lfo2oX, lfo2oY);
				lfo2oX2 = lfo2oX;
				lfo2oX = lfo2oX + t_lfo2Width;
			}
			break;
		case 1:
			lfo2OriginX = lfo2OriginStartX + (lfo2Slider * (t_lfo2Width)) - t_lfo2Width * .5;
			lfo2oX = lfo2OriginX + t_lfo2Width;
			lfo2oY = lfo2OriginY;
			lfo2oX2 = lfo2OriginX;
			lfo2oX2Handle = lfo2oX2 + (t_lfo2Width*.5);
			lfo2Height = lfoMinHeight + lfoHeight * lfo2Amp;
			lfo2Height = lfo2Height * .35;
			for (int i = 0; i <= (*lfo2Rate) + 2; i++) {
				nvgLineTo(vg,
					lfo2oX2,
					(lfo2oY - lfo2Height)
				);
				nvgLineTo(vg,
					lfo2oX,
					(lfo2oY + lfo2Height)
				);
				lfo2oX2 = lfo2oX;
				lfo2oX = lfo2oX + t_lfo2Width;
			}
			break;
		case 2:
			lfo2OriginX = lfo2OriginStartX + (lfo2Slider * (t_lfo2Width)) - t_lfo2Width * .5;
			lfo2oX = lfo2OriginX + t_lfo2Width;
			lfo2oY = lfo2OriginY;
			lfo2oX2 = lfo2OriginX;
			lfo2oX2Handle = lfo2oX2 + (t_lfo2Width*.5);
			lfo2Height = lfoMinHeight + lfoHeight * lfo2Amp;
			lfo2Height = lfo2Height * .35;
			nvgMoveTo(vg, lfo2OriginX, lfo2OriginY + lfo2Height);
			for (int i = 0; i <= (*lfo2Rate) + 2; i++) {
				nvgLineTo(vg,
					lfo2oX2 + (t_lfo2Width*.5),
					(lfo2oY - lfo2Height)
				);
				nvgLineTo(vg,
					lfo2oX,
					(lfo2oY + lfo2Height)
				);
				lfo2oX2 = lfo2oX;
				lfo2oX = lfo2oX + t_lfo2Width;
			}
			break;
		case 3:
			lfo2OriginX = lfo2OriginStartX + (lfo2Slider * (t_lfo2Width)) - t_lfo2Width * .5;
			lfo2oX = lfo2OriginX + t_lfo2Width;
			lfo2oY = lfo2OriginY;
			lfo2oX2 = lfo2OriginX;
			lfo2oX2Handle = lfo2oX2 + (t_lfo2Width*.5);
			lfo2Height = lfoMinHeight + lfoHeight * lfo2Amp;
			lfo2Height = lfo2Height * .35;
			nvgMoveTo(vg, lfo2OriginX, lfo2OriginY + lfo2Height);
			for (int i = 0; i <= (*lfo2Rate) + 2; i++) {
				nvgLineTo(vg,
					lfo2oX2,
					(lfo2oY - lfo2Height)
				);
				nvgLineTo(vg,
					lfo2oX2 + (t_lfo2Width*.5),
					(lfo2oY - lfo2Height)
				);
				nvgLineTo(vg,
					lfo2oX2 + (t_lfo2Width*.5),
					(lfo2oY + lfo2Height)
				);
				nvgLineTo(vg,
					lfo2oX,
					(lfo2oY + lfo2Height)
				);
				lfo2oX2 = lfo2oX;
				lfo2oX = lfo2oX + t_lfo2Width;
			}
			break;
		}
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, 2.0F);
		nvgStroke(vg);

		nvgClosePath(vg);
		nvgResetScissor(vg);
	}

	void NVGView::drawEnv6() {
		lfo3Amp = knobMap.at("lfo3Amp")->getValueNormalized();
		lfo3Type = int64(knobMap.at("lfo3Type")->getValue());
		//lfo3Rate supplied by message notify from processor
		lfo3Slider = sliderMap.at("lfo3PhaseSlider")->getValue() - 0.5;
		t_lfo3Width = lfoWidth / fmax(0.03, (*lfo3Rate));
		//lfo3OriginX = lfo3OriginStartX + (lfo3Slider * (t_lfo3Width)) - (t_lfo3Width*.25) - (ceil(lfo3Slider) * t_lfo3Width);

		nvgBeginPath(vg);
		nvgScissor(vg, 0, 0, 1148, 542);
		nvgIntersectScissor(vg, lfo3OriginStartX, lfo3OriginY - lfoHeight / 2, lfoWidth, lfoHeight);
		nvgMoveTo(vg, lfo3OriginX, lfo3OriginY);
		switch (lfo3Type) {
		case 0:
			lfo3OriginX = lfo3OriginStartX + (lfo3Slider * (t_lfo3Width)) - t_lfo3Width * .5;
			lfo3oX = lfo3OriginX + t_lfo3Width;
			lfo3oY = lfo3OriginY;
			lfo3oX2 = lfo3OriginX;
			lfo3oX2Handle = lfo3oX2 + (t_lfo3Width*.5);
			lfo3Height = lfoMinHeight + lfoHeight * lfo3Amp;
			lfo3Height = lfo3Height * 1.25;
			for (int i = 0; i <= (*lfo3Rate) + 2; i++) {
				lfo3oX2Handle = lfo3oX2 + (t_lfo3Width*.5);
				nvgBezierTo(vg,
					(lfo3oX2Handle),
					(lfo3oY - lfo3Height),
					(lfo3oX2Handle),
					(lfo3oY + lfo3Height),
					lfo3oX, lfo3oY);
				lfo3oX2 = lfo3oX;
				lfo3oX = lfo3oX + t_lfo3Width;
			}
			break;
		case 1:
			lfo3OriginX = lfo3OriginStartX + (lfo3Slider * (t_lfo3Width)) - t_lfo3Width * .5;
			lfo3oX = lfo3OriginX + t_lfo3Width;
			lfo3oY = lfo3OriginY;
			lfo3oX2 = lfo3OriginX;
			lfo3oX2Handle = lfo3oX2 + (t_lfo3Width*.5);
			lfo3Height = lfoMinHeight + lfoHeight * lfo3Amp;
			lfo3Height = lfo3Height * .35;
			for (int i = 0; i <= (*lfo3Rate) + 2; i++) {
				nvgLineTo(vg,
					lfo3oX2,
					(lfo3oY - lfo3Height)
				);
				nvgLineTo(vg,
					lfo3oX,
					(lfo3oY + lfo3Height)
				);
				lfo3oX2 = lfo3oX;
				lfo3oX = lfo3oX + t_lfo3Width;
			}
			break;
		case 2:
			lfo3OriginX = lfo3OriginStartX + (lfo3Slider * (t_lfo3Width)) - t_lfo3Width * .5;
			lfo3oX = lfo3OriginX + t_lfo3Width;
			lfo3oY = lfo3OriginY;
			lfo3oX2 = lfo3OriginX;
			lfo3oX2Handle = lfo3oX2 + (t_lfo3Width*.5);
			lfo3Height = lfoMinHeight + lfoHeight * lfo3Amp;
			lfo3Height = lfo3Height * .35;
			nvgMoveTo(vg, lfo3OriginX, lfo3OriginY + lfo3Height);
			for (int i = 0; i <= (*lfo3Rate) + 2; i++) {
				nvgLineTo(vg,
					lfo3oX2 + (t_lfo3Width*.5),
					(lfo3oY - lfo3Height)
				);
				nvgLineTo(vg,
					lfo3oX,
					(lfo3oY + lfo3Height)
				);
				lfo3oX2 = lfo3oX;
				lfo3oX = lfo3oX + t_lfo3Width;
			}
			break;
		case 3:
			lfo3OriginX = lfo3OriginStartX + (lfo3Slider * (t_lfo3Width)) - t_lfo3Width * .5;
			lfo3oX = lfo3OriginX + t_lfo3Width;
			lfo3oY = lfo3OriginY;
			lfo3oX2 = lfo3OriginX;
			lfo3oX2Handle = lfo3oX2 + (t_lfo3Width*.5);
			lfo3Height = lfoMinHeight + lfoHeight * lfo3Amp;
			lfo3Height = lfo3Height * .35;
			nvgMoveTo(vg, lfo3OriginX, lfo3OriginY + lfo3Height);
			for (int i = 0; i <= (*lfo3Rate) + 2; i++) {
				nvgLineTo(vg,
					lfo3oX2,
					(lfo3oY - lfo3Height)
				);
				nvgLineTo(vg,
					lfo3oX2 + (t_lfo3Width*.5),
					(lfo3oY - lfo3Height)
				);
				nvgLineTo(vg,
					lfo3oX2 + (t_lfo3Width*.5),
					(lfo3oY + lfo3Height)
				);
				nvgLineTo(vg,
					lfo3oX,
					(lfo3oY + lfo3Height)
				);
				lfo3oX2 = lfo3oX;
				lfo3oX = lfo3oX + t_lfo3Width;
			}
			break;
		}
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, 2.0F);
		nvgStroke(vg);

		nvgClosePath(vg);
		nvgResetScissor(vg);
	}

	void NVGView::drawEnv1() {
		env1Att = knobMap.at("env1Att")->getValueNormalized();
		env1Dec = knobMap.at("env1Dec")->getValueNormalized();
		env1Rel = knobMap.at("env1Rel")->getValueNormalized();
		env1Toggle = buttonMap.at("env1DecToggle")->getValueNormalized();
		
		nvgBeginPath(vg);
		nvgMoveTo(vg, env1OriginX, env1OriginY);//IMPLICIT CONVERSION WATCH OUT
		env1AttX = env1OriginX + (envXDif * env1Att);
		env1AttY = env1OriginY - envYDif;
		if (env1Dec > 0.6) {
			//env1AttX = env1AttX + (env1AttX* fmax(0.0, env1Dec - 0.6));
			env1AttY = env1AttY + ((envYDif* fmax(0.0, env1Dec - 0.6))*env1Att);
		}
		
		env1RelXSize = ((envXDif * env1Rel) * ((env1Toggle - 1) * -1));
		env1DecX = env1AttX + envXBuffer + (envXDif * env1Dec + (env1Dec * envXDif*2) );
		if (env1Dec == 1.0) {
			env1DecY = env1AttY = env1OriginY - envYDif;
			env1DecX = envFinalX + envXBuffer + env1OriginX - env1RelXSize;
		}
		else {
			env1DecY = env1OriginY - (22 * ( ( env1Toggle-1) * -1 ));
			env1DecX = fmin(envFinalX + envXBuffer + env1OriginX - env1RelXSize , fmax(env1AttX, (env1DecX - env1RelXSize) ));
		}
		nvgLineTo(vg, env1AttX, env1AttY);
		nvgLineTo(vg, env1DecX, env1DecY);
		env1RelX = env1DecX + ((envXDif * env1Rel) * ((env1Toggle - 1) * -1) );
		env1RelY = env1OriginY;
		nvgLineTo(vg, env1RelX, env1RelY);
		nvgLineTo(vg, env1OriginX, env1OriginY);
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, 2.0F);
		nvgStroke(vg);
		nvgClosePath(vg);
		nvgFillColor(vg, nvgRGBA(0, 160, 192, 225));//( 255 * cBBtn->getValue() ) should be 0 off or 255 on;
		nvgFill(vg);
		
	}

	void NVGView::drawEnv2() {
		env2Att = knobMap.at("env2Att")->getValueNormalized();
		env2Dec = knobMap.at("env2Dec")->getValueNormalized();
		env2Rel = knobMap.at("env2Rel")->getValueNormalized();
		env2Toggle = buttonMap.at("env2DecToggle")->getValueNormalized();

		nvgBeginPath(vg);
		nvgMoveTo(vg, env2OriginX, env2OriginY);//IMPLICIT CONVERSION WATCH OUT
		env2AttX = env2OriginX + (envXDif * env2Att);
		env2AttY = env2OriginY - envYDif;
		if (env2Dec > 0.6) {
			env2AttY = env2AttY + ((envYDif* fmax(0.0, env2Dec - 0.6))*env2Att);
		}
		env2RelXSize = ((envXDif * env2Rel) * ((env2Toggle - 1) * -1));
		env2DecX = env2AttX + envXBuffer + (envXDif * env2Dec + (env2Dec * envXDif * 2));
		if (env2Dec == 1.0) {
			env2DecY = env2AttY;

			env2DecX = envFinalX + envXBuffer + env2OriginX - env2RelXSize;
		}
		else {
			env2DecY = env2OriginY - (22 * ((env2Toggle - 1) * -1));
			env2DecX = fmin(envFinalX + envXBuffer + env2OriginX - env2RelXSize, fmax(env2AttX, (env2DecX - env2RelXSize)));
		}
		nvgLineTo(vg, env2AttX, env2AttY);
		nvgLineTo(vg, env2DecX, env2DecY);
		env2RelX = env2DecX + ((envXDif * env2Rel) * ((env2Toggle - 1) * -1));
		env2RelY = env2OriginY;
		nvgLineTo(vg, env2RelX, env2RelY);
		nvgLineTo(vg, env2OriginX, env2OriginY);
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, 2.0F);
		nvgStroke(vg);
		nvgClosePath(vg);
		nvgFillColor(vg, nvgRGBA(0, 160, 192, 225));//( 255 * cBBtn->getValue() ) should be 0 off or 255 on;
		nvgFill(vg);

	}
	 
	void NVGView::drawEnv3() {
		env3Att = knobMap.at("env3Att")->getValueNormalized();
		env3Dec = knobMap.at("env3Dec")->getValueNormalized();
		env3Rel = knobMap.at("env3Rel")->getValueNormalized();
		env3Toggle = buttonMap.at("env3DecToggle")->getValueNormalized();

		nvgBeginPath(vg);
		nvgMoveTo(vg, env3OriginX, env3OriginY);//IMPLICIT CONVERSION WATCH OUT
		env3AttX = env3OriginX + (envXDif * env3Att);
		env3AttY = env3OriginY - envYDif;
		if (env3Dec > 0.6) {
			env3AttY = env3AttY + ((envYDif* fmax(0.0, env3Dec - 0.6))*env3Att);
		}
		env3RelXSize = ((envXDif * env3Rel) * ((env3Toggle - 1) * -1));
		env3DecX = env3AttX + envXBuffer + (envXDif * env3Dec + (env3Dec * envXDif * 2));
		if (env3Dec == 1.0) {
			env3DecY = env3AttY;

			env3DecX = envFinalX + envXBuffer + env3OriginX - env3RelXSize;
		}
		else {
			env3DecY = env3OriginY - (22 * ((env3Toggle - 1) * -1));
			env3DecX = fmin(envFinalX + envXBuffer + env3OriginX - env3RelXSize, fmax(env3AttX, (env3DecX - env3RelXSize)));
		}
		nvgLineTo(vg, env3AttX, env3AttY);
		nvgLineTo(vg, env3DecX, env3DecY);
		env3RelX = env3DecX + ((envXDif * env3Rel) * ((env3Toggle - 1) * -1));
		env3RelY = env3OriginY;
		nvgLineTo(vg, env3RelX, env3RelY);
		nvgLineTo(vg, env3OriginX, env3OriginY);
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, 2.0F);
		nvgStroke(vg);
		nvgClosePath(vg);
		nvgFillColor(vg, nvgRGBA(0, 160, 192, 225));//( 255 * cBBtn->getValue() ) should be 0 off or 255 on;
		nvgFill(vg);

	}


	void NVGView::drawSlider(RData& reIn) {
		sldrOb = &RViewsMap.at(reIn.dataName);
		cBSldr = sliderMap.at(reIn.dataName);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, sldrOb->drawRect.getTopLeft().x, sldrOb->drawRect.getTopLeft().y, sldrOb->drawRect.getWidth(), sldrOb->drawRect.getHeight(), 3.0f);
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
		nvgClosePath(vg);
		
		nvgBeginPath(vg);
		nvgRoundedRect(vg, ((sldrOb->drawRect.getTopLeft().x + 3) + ((sldrOb->drawRect.getWidth() - 6)/2)), sldrOb->drawRect.getTopLeft().y + 3, ((sldrOb->drawRect.getWidth() - 6) * (cBSldr->getValue() - 0.5f )) , sldrOb->drawRect.getHeight() - 6, 3.0f);
		//draw rect of desired width * (slider value - .5f) at x+(desired width/2)
		
		nvgFillColor(vg, nvgRGBA(0, 160, 192, 225));//( 255 * cBBtn->getValue() ) should be 0 off or 255 on;
		nvgFill(vg);
		nvgClosePath(vg);
	}



	void NVGView::drawSliderVert(RData& reIn) {
		
		sldrOb = &RViewsMap.at(reIn.dataName);
		cBSldr = sliderMap.at(reIn.dataName);
		 
		nvgBeginPath(vg);
		nvgRoundedRect(vg, sldrOb->drawRect.getTopLeft().x, sldrOb->drawRect.getTopLeft().y, sldrOb->drawRect.getWidth(), sldrOb->drawRect.getHeight(), 3.0f);
		nvgFillColor(vg, nvgRGBA(0, 0, 0, 225));//( 255 * cBBtn->getValue() ) should be 0 off or 255 on;
		nvgFill(vg);
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
		nvgClosePath(vg);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, sldrOb->drawRect.getTopLeft().x + 3, ((sldrOb->drawRect.getTopLeft().y + 3) + ((sldrOb->drawRect.getHeight() - 6) / 2)), sldrOb->drawRect.getWidth() - 6, ((sldrOb->drawRect.getHeight() - 6) * (cBSldr->getValue() - 0.5f)), 3.0f);
		//draw rect of desired width * (slider value - .5f) at x+(desired width/2)

		nvgFillColor(vg, nvgRGBA(0, 160, 192, 225));//( 255 * cBBtn->getValue() ) should be 0 off or 255 on;
		nvgFill(vg);
		nvgClosePath(vg);
		
	}


	void NVGView::drawSwitchVert(RData& reIn) {
		vSwxOb = &RViewsMap.at(reIn.dataName);
		cBVswx = vertSwitchMap.at(reIn.dataName);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, vSwxOb->drawRect.getTopLeft().x, vSwxOb->drawRect.getTopLeft().y, vSwxOb->drawRect.getWidth(), vSwxOb->drawRect.getHeight(), 3.0f);
		//nvgFillColor(vg, nvgRGBA(0, 0, 0, 225));//( 255 * cBBtn->getValue() ) should be 0 off or 255 on;
		//nvgFill(vg);
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
		nvgClosePath(vg);
		float val = cBVswx->getValue();
		float max = cBVswx->getMax();
		nvgLineJoin(vg, NVG_ROUND);
		nvgLineCap(vg, NVG_ROUND);
		for (float i = 0.0; i <= max; i = i+1.0) {
			nvgBeginPath(vg);
			int yInt = (int)(((vSwxOb->drawRect.getBottomLeft().y - (vSwxOb->height)*i) - (vSwxOb->height - ((i / cBVswx->getMax())*vSwxOb->height / 2))))+2;
			nvgRoundedRect(vg, vSwxOb->drawRect.getTopLeft().x + 3, yInt, vSwxOb->drawRect.getWidth() - 6, 1, 0.0f);
			nvgFillColor(vg, nvgRGB(0, 51, 51));
			nvgFill(vg);
			nvgClosePath(vg);

		}
		nvgLineJoin(vg, NVG_MITER);
		nvgLineCap(vg, NVG_BUTT);
		nvgBeginPath(vg);
		int yInt = (int)(((vSwxOb->drawRect.getBottomLeft().y - (vSwxOb->height)*cBVswx->getValue()) - (vSwxOb->height - ((cBVswx->getValue() / cBVswx->getMax())*vSwxOb->height / 2)))) +1;
		nvgRoundedRect(vg, vSwxOb->drawRect.getTopLeft().x + 3, yInt, vSwxOb->drawRect.getWidth() - 6, 2, 0.0f);
		nvgFillColor(vg, nvgRGB(170, 66, 133));
		nvgFill(vg);
		nvgClosePath(vg);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, vSwxOb->drawRect.getTopLeft().x, vSwxOb->drawRect.getTopLeft().y, vSwxOb->drawRect.getWidth(), vSwxOb->drawRect.getHeight(), 3.0f);
		nvgFillColor(vg, nvgRGBA(170, 66, 133, (vSwxOb->alpha*vSwxOb->blinkIntensity)));
		nvgFill(vg);
		nvgClosePath(vg);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, vSwxOb->drawRect.getTopLeft().x, vSwxOb->drawRect.getTopLeft().y, vSwxOb->drawRect.getWidth(), vSwxOb->drawRect.getHeight(), 3.0f);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, (255 * vSwxOb->blinkIntensity)));
		nvgStrokeWidth(vg, 3.0f);
		nvgStroke(vg);
		nvgClosePath(vg);

	}

	void NVGView::drawSelctedStepTarg() {
		nvgBeginPath(vg);
		if (selectedStepTarg->RSBType == 1) {
			nvgRoundedRect(vg, selectedStepTarg->drawRect.getTopLeft().x, selectedStepTarg->drawRect.getTopLeft().y, selectedStepTarg->drawRect.getWidth(), selectedStepTarg->drawRect.getHeight(), 3.0f);
		}
		if (selectedStepTarg->RSBType == 2) {
			nvgCircle(vg, selectedStepTarg->drawRect.getCenter().x, selectedStepTarg->drawRect.getCenter().y, (selectedStepTarg->drawRect.getWidth() / 2.0));
		}
		
		nvgStrokeColor(vg, nvgRGB(255, 255, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
		nvgClosePath(vg);
	}

	void NVGView::drawStepCtrlBtn(RData& reIn) {
		btnOb = &RViewsMap.at(reIn.dataName);
		cBStepCtrl = stepCtrlButtonMap.at(reIn.dataName);
		nvgBeginPath(vg);
		nvgMoveTo(vg, btnOb->baseX, btnOb->baseY);
		nvgLineTo(vg, btnOb->baseX, btnOb->lowerRightY);
		nvgLineTo(vg, btnOb->lowerLeftX, btnOb->lowerLeftY);
		nvgLineTo(vg, btnOb->upperLeftX1, btnOb->upperLeftY1);
		nvgLineTo(vg, btnOb->upperLeftX2, btnOb->upperLeftY2);
		nvgLineTo(vg, btnOb->baseX, btnOb->baseY);
		nvgFillColor(vg, nvgRGBA(170, 66, 133, (btnOb->alpha*btnOb->blinkIntensity)*2.0 ));
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, (190.0*btnOb->blinkIntensity)));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
		nvgFill(vg);
		nvgClosePath(vg);
	}
	
	void NVGView::drawButton(RData& reIn) {
		btnOb = &RViewsMap.at(reIn.dataName);
		cBBtn = buttonMap.at(reIn.dataName);
		
		glowPaint = nvgRadialGradient(vg, cBBtn->getViewSize().getCenter().x, cBBtn->getViewSize().getCenter().y, 5, btnOb->drawRect.getWidth(), nvgRGBA(245, 40, 145, (55 * btnOb->blinker)), nvgRGBA(245, 40, 145, (0 * btnOb->blinker)));

		//draw main button
		nvgBeginPath(vg);
		nvgRoundedRect(vg, btnOb->drawRect.getTopLeft().x, btnOb->drawRect.getTopLeft().y, btnOb->drawRect.getWidth(), btnOb->drawRect.getHeight(), 3.0f );
		nvgFillColor(vg, nvgRGBA(0, 0, 0, 225));//( 255 * cBBtn->getValue() ) should be 0 off or 255 on;
		nvgFill(vg);
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
		nvgClosePath(vg);

		//glow
		nvgBeginPath(vg);
		nvgCircle(vg, cBBtn->getViewSize().getCenter().x, cBBtn->getViewSize().getCenter().y, btnOb->drawRect.getWidth() );
		nvgFillPaint(vg, glowPaint);
		nvgClosePath(vg);
		nvgFill(vg);
		 
		//draw button "on" light
		nvgBeginPath(vg);
		nvgRoundedRect(vg, btnOb->drawRect.getTopLeft().x+1, btnOb->drawRect.getTopLeft().y+1, btnOb->drawRect.getWidth()-2, btnOb->drawRect.getHeight()-2, 2.0f);
		nvgFillColor(vg, nvgRGBA(245, 40, 145, ( 255 * btnOb->blinker)));
		nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255));
		nvgStrokeWidth(vg, 1.0f);
		nvgFill(vg);
		nvgStroke(vg);
		nvgClosePath(vg);

		//nvgFontSize(vg, 9.0f);
		//nvgFontFace(vg, "lucon");
		//nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		//nvgFillColor(vg, nvgRGBA(0, 160, 192, 255));
		/*
		nvgText(vg, 
			btnOb->drawRect.getTopRight().x + ( reIn.dataName.length()  * 3)
			, btnOb->drawRect.getTopRight().y + ( (btnOb->drawRect.getHeight() / 2.0) + 3 )
			, reIn.dataName.c_str(), NULL);
			*/

	}

	void NVGView::drawCButton(RData& reIn) {
		clickBtnOb = &RViewsMap.at(reIn.dataName);
		clickCBBtn = cButtonMap.at(reIn.dataName);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, clickBtnOb->drawRect.getTopLeft().x, clickBtnOb->drawRect.getTopLeft().y, clickBtnOb->drawRect.getWidth(), clickBtnOb->drawRect.getHeight(), 1.0f);
		nvgFillColor(vg, nvgRGBA(0, 0, 0, 225));//( 255 * clickcBBtn->getValue() ) should be 0 off or 255 on;
		nvgFill(vg);
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);
		nvgClosePath(vg);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, clickBtnOb->drawRect.getTopLeft().x + 1, clickBtnOb->drawRect.getTopLeft().y + 1, clickBtnOb->drawRect.getWidth() - 2, clickBtnOb->drawRect.getHeight() - 2, 1.0f);
		nvgFillColor(vg, nvgRGBA(165, 248, 211, (255 * clickBtnOb->blinkIntensity)));//( 255 * clickcBBtn->getValue() ) should be 0 off or 255 on;
		//nvgFillPaint(vg, nvgRadialGradient(vg, clickBtnOb->drawRect.getTopLeft().x + 1, clickBtnOb->drawRect.getTopLeft().y + 1, clickBtnOb->drawRect.getWidth() - 2, clickBtnOb->drawRect.getHeight() - 2, nvgRGBA(165, 248, 211, (100 * clickBtnOb->blinkIntensity)), nvgRGBA(165, 248, 211, (10 * clickBtnOb->blinkIntensity))));
		nvgClosePath(vg);
		nvgFill(vg);
		//nvgFontSize(vg, 9.0f);
		//nvgFontFace(vg, "lucon");
		//nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		//nvgFillColor(vg, nvgRGBA(0, 160, 192, 255));
		/*
		nvgText(vg,
			btnOb->drawRect.getTopRight().x + ( reIn.dataName.length()  * 3)
			, btnOb->drawRect.getTopRight().y + ( (btnOb->drawRect.getHeight() / 2.0) + 3 )
			, reIn.dataName.c_str(), NULL);
			*/

	}

	void NVGView::drawBeatModOn(){
		if (beatModOnLev < .95f) {
			beatModOnLev += .05f * beatModOn;
		}

		nvgBeginPath(vg);
		nvgCircle(vg, 758, 261, 3);
		nvgFillColor(vg, nvgRGBA(245, 40, 145, (255 * beatModOnLev)));
		nvgClosePath(vg);
		nvgFill(vg);

		lightPaint = nvgRadialGradient(vg, 758, 261, 3, 7, nvgRGBA(245, 40, 145, (100 * beatModOnLev)), nvgRGBA(245, 40, 145, (0 * beatModOnLev)));

		nvgBeginPath(vg);
		nvgCircle(vg, 758, 261, 7);
		nvgFillPaint(vg, lightPaint);
		nvgClosePath(vg);
		nvgFill(vg);

		
	}


	void NVGView::drawBadReg() {

		nvgBeginPath(vg);
		nvgRect(vg, 237,90, 646, 341);
		nvgFillColor(vg, nvgRGBA(0, 0, 0, 235));
		nvgStrokeColor(vg, nvgRGBA(255, 102, 0, 255));
		nvgStrokeWidth(vg, 1.0f);
		nvgClosePath(vg);
		nvgFill(vg);
		nvgStroke(vg);
		float bounds[4];
		nvgFontSize(vg, 13.0f);
		nvgFontFace(vg, "lucon");
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, nvgRGB(255, 153, 255));
		nvgText(vg, 560, 230, "No registration detected! ", NULL);
		nvgText(vg, 560, 260, "Please go on-line to rogue-syntax.com and register.", NULL);
		nvgText(vg, 560, 290, "Registration is quick, easy, and dirt cheap!", NULL);
	}


	void NVGView::drawSineSymbol(RData& reIn) {
		viewOb = &RViewsMap.at(reIn.dataName);
		cBView = viewMap.at(reIn.dataName);
		NVGpaint imgPaint;
		imgPaint = nvgImagePattern(vg, viewOb->drawRect.getTopLeft().x, viewOb->drawRect.getTopLeft().y, viewOb->drawRect.getWidth(), viewOb->drawRect.getHeight(), 0.0f / 180.0f*NVG_PI, images[viewOb->viewType], 1.0);
		nvgBeginPath(vg);
		nvgRoundedRect(vg, viewOb->drawRect.getTopLeft().x, viewOb->drawRect.getTopLeft().y, viewOb->drawRect.getWidth(), viewOb->drawRect.getHeight(), 0.0);
		nvgFillPaint(vg, imgPaint);
		nvgFill(vg);
		/*
		nvgBeginPath(vg);

		nvgCircle(vg, viewOb->drawRect.getCenter().x, viewOb->drawRect.getCenter().y, (viewOb->drawRect.getWidth() / 2.0));
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
		nvgClosePath(vg);

		nvgBeginPath(vg);
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, 2.0f);
		

		float x = 4.0;
		float y = 0;
		float width = 20;
		float height = 20;
		float scale = 1;
		float amplitude = 4;
		float frequency = 2;
		//IMPLICIT CONVERSION WATCH OUT
		//gotta do th setup do we haev 
		
		while (x < width) {

			y = height / 2 + amplitude * sin(x / frequency);
			if (x == 4.0) {
				nvgMoveTo(vg, viewOb->drawRect.getTopLeft().x + x, viewOb->drawRect.getTopLeft().y + y);
			}

			nvgLineTo(vg, viewOb->drawRect.getTopLeft().x + x, viewOb->drawRect.getTopLeft().y + y);

			x = x + 1;
		}
		nvgStroke(vg);
		nvgClosePath(vg);

		*/
	}

	void NVGView::drawKnob(RData& reIn) {

		knobOb = &RViewsMap.at(reIn.dataName);
		cBKnob = knobMap.at(reIn.dataName);

		/*
		NVGpaint imgPaint;
		imgPaint = nvgImagePattern(vg, knobOb->drawRect.getTopLeft().x, knobOb->drawRect.getTopLeft().y, knobOb->drawRect.getWidth(), knobOb->drawRect.getHeight(), 0.0f / 180.0f*NVG_PI, images[knobOb->viewType], 1.0);
		nvgBeginPath(vg);
		nvgRoundedRect(vg, knobOb->drawRect.getTopLeft().x, knobOb->drawRect.getTopLeft().y, knobOb->drawRect.getWidth(), knobOb->drawRect.getHeight(), 0.0);
		nvgFillPaint(vg, imgPaint);
		nvgFill(vg);
		nvgClosePath(vg);
		*/
		
		nvgBeginPath(vg);
		nvgCircle(vg, cBKnob->getViewSize().getCenter().x, cBKnob->getViewSize().getCenter().y,  (knobOb->drawRect.getWidth()/2.0) );
		nvgStrokeColor(vg, nvgRGB(4, 186, 232));
		nvgStrokeWidth(vg, 3.0f);
		nvgStroke(vg);
		nvgClosePath(vg);
	 
		nvgBeginPath(vg);
		nvgCircle(vg, cBKnob->getViewSize().getCenter().x, cBKnob->getViewSize().getCenter().y, (knobOb->drawRect.getWidth() / 2.0));
		nvgFillColor(vg, nvgRGBA(170, 66, 133, (knobOb->alpha*knobOb->blinkIntensity)));
		nvgFill(vg);
		nvgClosePath(vg);

		
		/*
		nvgFontSize(vg, 9.0f);
		nvgFontFace(vg, "lucon");
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFillColor(vg, nvgRGB(0, 160, 192) );
		nvgText(vg, 975, 273, envSrcNames[selectedEnvSrc], NULL);
		*/
		//make ctrl handles
		CPoint cpt;// make meber
		cBKnob->valueToPoint(cpt);
		CPoint origin( (cBKnob->getViewSize().getWidth() / 2), (cBKnob->getViewSize().getHeight() / 2) );//make member
		cpt.offset( cBKnob->getViewSize().left, cBKnob->getViewSize().top );
		origin.offset( cBKnob->getViewSize().left, cBKnob->getViewSize().top );

		nvgBeginPath(vg);
		nvgMoveTo(vg, origin.x, origin.y);//IMPLICIT CONVERSION WATCH OUT
		nvgLineTo(vg, cpt.x, cpt.y);
		nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		nvgStrokeWidth(vg, knobOb->strokeWidth1);
		nvgStroke(vg);
		nvgClosePath(vg);
		/*
		if (knobOb->overlayType != NONE) {
			imgPaint = nvgImagePattern(vg, knobOb->drawRect.getCenter().x - (knobOb->overlayWidth / 2), knobOb->drawRect.getCenter().y - (knobOb->overlayHeight / 2), knobOb->overlayWidth, knobOb->overlayHeight, 0.0f / 180.0f*NVG_PI, images[knobOb->overlayType], 1.0);
			nvgBeginPath(vg);
			nvgRoundedRect(vg, knobOb->drawRect.getCenter().x- (knobOb->overlayWidth / 2), knobOb->drawRect.getCenter().y-(knobOb->overlayHeight/2), knobOb->overlayWidth, knobOb->overlayHeight, 0.0);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
			nvgClosePath(vg);
		}
		*/
		nvgBeginPath(vg);
		nvgCircle(vg, cBKnob->getViewSize().getCenter().x, cBKnob->getViewSize().getCenter().y, (knobOb->drawRect.getWidth() / 2.0));
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, (255*knobOb->blinkIntensity)));
		nvgStrokeWidth(vg, 3.0f);
		nvgStroke(vg);
		nvgClosePath(vg);
		
	}

	void NVGView::drawKnob2(RData& reIn) {

		knobOb = &RViewsMap.at(reIn.dataName);
		cBKnob = knobMap.at(reIn.dataName);

		nvgBeginPath(vg);
		nvgCircle(vg, cBKnob->getViewSize().getCenter().x, cBKnob->getViewSize().getCenter().y, (knobOb->drawRect.getWidth() / 2.0));
		nvgStrokeColor(vg, nvgRGB(0, 254, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
		nvgClosePath(vg);

		/*nvgBeginPath(vg);
		nvgCircle(vg, cBKnob->getViewSize().getCenter().x, cBKnob->getViewSize().getCenter().y, (knobOb->drawRect.getWidth() / 2.0));
		nvgFillColor(vg, nvgRGBA(170, 66, 133, (knobOb->alpha*knobOb->blinkIntensity)));
		nvgFill(vg);
		nvgClosePath(vg);*/

		CPoint cpt;// make meber
		cBKnob->valueToPoint(cpt);
		CPoint origin((cBKnob->getViewSize().getWidth() / 2), (cBKnob->getViewSize().getHeight() / 2));//make member
		cpt.offset(cBKnob->getViewSize().left, cBKnob->getViewSize().top);
		origin.offset(cBKnob->getViewSize().left, cBKnob->getViewSize().top);

		nvgBeginPath(vg);
		nvgMoveTo(vg, origin.x, origin.y);//IMPLICIT CONVERSION WATCH OUT
		nvgLineTo(vg, cpt.x, cpt.y);
		nvgStrokeColor(vg, nvgRGB(0, 254, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
		nvgClosePath(vg);
		
		/*nvgBeginPath(vg);
		nvgCircle(vg, cBKnob->getViewSize().getCenter().x, cBKnob->getViewSize().getCenter().y, (knobOb->drawRect.getWidth() / 2.0));
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, (255 * knobOb->blinkIntensity)));
		nvgStrokeWidth(vg, 3.0f);
		nvgStroke(vg);
		nvgClosePath(vg);*/

	}

	void NVGView::drawCtrlValueOutput() {

	
		targDisplayTxt.processText();
		targValueTxt.processText();

		nvgFontSize(vg, 11.0f);
		nvgFontFace(vg, "lucon");
		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(255, 204, 153, 255 * targDisplayTxt.textDisplayDecay));
		//nvgText(vg, 980, 248, selectedCtrlTarg, NULL);
		nvgText(vg, 980, 248, targDisplayTxt.selectedCtrlTargBuilder.c_str(), NULL);
		nvgText(vg, 980, 283, targValueTxt.selectedCtrlTargBuilder.c_str(), NULL);
	}

	void NVGView::drawImage(RData& reIn) {
		viewOb = &RViewsMap.at(reIn.dataName);
		cBView = viewMap.at(reIn.dataName);
		NVGpaint imgPaint;
		imgPaint = nvgImagePattern(vg, viewOb->drawRect.getTopLeft().x, viewOb->drawRect.getTopLeft().y, viewOb->drawRect.getWidth(), viewOb->drawRect.getHeight(), 0.0f / 180.0f*NVG_PI, images[viewOb->viewType], 1.0);
		nvgBeginPath(vg);
		nvgRoundedRect(vg, viewOb->drawRect.getTopLeft().x, viewOb->drawRect.getTopLeft().y, viewOb->drawRect.getWidth(), viewOb->drawRect.getHeight(), 0.0);
		nvgFillPaint(vg, imgPaint);
		nvgFill(vg);
		
	}


	void NVGView::drawBkg(RData& reIn) {
		//RViewsMap[reIn.eventName]->x = 0;


		NVGpaint imgPaint;
		//imgPaint = nvgImagePattern(vg, 0.0, 0.0, 1148.0, 542.0, 0.0f / 180.0f*NVG_PI, images[MAIN_ROW_BKG], 1.0);
		imgPaint = nvgImagePattern(vg, 0, 0, 1148, 542, 0.0f, images[MAIN_ROW_BKG], 1.0);
		nvgBeginPath(vg);
		nvgRoundedRect(vg,0, 0, 1148, 542, 0.0);
		nvgFillPaint(vg, imgPaint);
		nvgFill(vg);
		nvgClosePath(vg);
		
		//nvgBeginFrame(vg, 940, 440, 1.0);

		//nvgBeginPath(vg);
		//nvgRect(vg, 5.0, 5.0, 1140.0, 540.0);
		//nvgStrokeColor(vg, nvgRGBA(0, 160, 192, 255));
		//nvgStrokeWidth(vg, 1.0f);
		//nvgStroke(vg);

		//nvgEndFrame(vg);
		//getPlatformOpenGLView()->swapBuffers();
	
			
		

		//glFlush();
		//getPlatformOpenGLView()->swapBuffers();
			

	};

	void NVGView::drawOGL() {
		GLPoint p1Scale;
		p1Scale.x = 0.15f;
		p1Scale.y = 0.15f;
		p1Scale.z = 0.15f;
		testPyr.trans2D(100.0f, 200.0f, 1148.0f, 542.0f);
		testPyr.scale(p1Scale);
		testPyr.draw();

		testPyr2.trans2D(400.0f, 200.0f, 1148.0f, 542.0f);
		testPyr2.scale(p1Scale);
		testPyr2.draw();
		/*
		xRotation += 1.f;
		if (xRotation >= 360.f)
			xRotation = 0.f;
		yRotation += 1.f;
		if (yRotation >= 360.f)
			yRotation = 0.f;
		zRotation += 1.f;
		if (zRotation >= 360.f)
			zRotation = 0.f;
		
		//xRotation = 45.0f;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glScaled(0.5, 0.5, 0.5);
		glRotated(xRotation, 1, 0, 0);
		glRotated(yRotation, 0, 1, 0);
		glRotated(zRotation, 0, 0, 1);

		glBegin(GL_TRIANGLES);

		glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
		glVertex3f(0.0f, 1.0f, 0.0f);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glVertex3f(1.0f, -1.0f, 1.0f);

		glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
		glVertex3f(0.0f, 1.0f, 0.0f);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glVertex3f(1.0f, -1.0f, -1.0f);

		glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
		glVertex3f(0.0f, 1.0f, 0.0f);
		glVertex3f(2.0f, -1.0f, -1.0f);
		glVertex3f(-1.5f, -1.0f, -1.0f);//1

		glColor4f(1.0f, 1.0f, 0.0f, 0.5f);
		glVertex3f(0.0f, 1.0f, 0.0f);
		glVertex3f(-1.5f, -1.0f, -1.0f);//1
		glVertex3f(-1.0f, -1.0f, 1.0f);

		glEnd();

		glBegin(GL_QUADS);
		glColor4f(0.5f, 0.5f, 0.5f, 0.9f);
		glVertex3f(-1.f, -1.f, 1.f);
		glVertex3f(1.f, -1.f, 1.f);
		glVertex3f(1.f, -1.f, -1.f);
		glVertex3f(-1.5f, -1.f, -1.f);//1
		glEnd();
		*/
	}


	void NVGView::drawMoreInfoBtn(RData& reIn) {
		RSBView* rsbView = &RViewsMap.at("MORE_INFO_BTN");
		//build info button
		nvgBeginPath(vg);
		nvgMoveTo(vg, moreInfoBtn->pathOutline[0].x, moreInfoBtn->pathOutline[0].y);
		nvgLineTo(vg, moreInfoBtn->pathOutline[1].x, moreInfoBtn->pathOutline[1].y);
		nvgLineTo(vg, moreInfoBtn->pathOutline[2].x, moreInfoBtn->pathOutline[2].y);
		nvgLineTo(vg, moreInfoBtn->pathOutline[3].x, moreInfoBtn->pathOutline[3].y);
		nvgLineTo(vg, moreInfoBtn->pathOutline[4].x, moreInfoBtn->pathOutline[4].y);
		nvgLineTo(vg, moreInfoBtn->pathOutline[5].x, moreInfoBtn->pathOutline[5].y);
		nvgLineTo(vg, moreInfoBtn->pathOutline[6].x, moreInfoBtn->pathOutline[6].y);
		nvgLineTo(vg, moreInfoBtn->pathOutline[7].x, moreInfoBtn->pathOutline[7].y);
		nvgLineTo(vg, moreInfoBtn->pathOutline[0].x, moreInfoBtn->pathOutline[0].y);

		nvgFillColor(vg, nvgRGBA(255, 255, 255, (255.0 * rsbView->blinkIntensity) ));
		nvgFill(vg);
		nvgClosePath(vg);

		std::string copyWriteStr = "BUILD INFO";
		nvgFontSize(vg, 11.0f);
		nvgFontFace(vg, "lucon");
		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(153, 102, 153, 255));
		nvgText(vg, 1000, 349, copyWriteStr.c_str(), NULL);
		//end build info button

		

	}

	
	std::string NVGView::getCopyright() {
		// Get the current year
		time_t now = time(0);
		tm* ltm = localtime(&now);
		int year = 1900 + ltm->tm_year;

		// Create the copyright string
		const std::string copyrightSymbol = "\u00A9";
		std::string yearStr = std::to_string(year);

		// Construct the result string
		std::string result = copyrightSymbol + yearStr + " Rogue Syntax Sound Design, LLC.";

		return result;
	}
	

	void NVGView::drawMoreInfoWindow() {
		//info window
		if (moreInfoWindowOpen) {
			
			NVGpaint imgPaint;
			//imgPaint = nvgImagePattern(vg, 0.0, 0.0, 1148.0, 542.0, 0.0f / 180.0f*NVG_PI, images[MAIN_ROW_BKG], 1.0);
			imgPaint = nvgImagePattern(vg, 0, 0, 1148, 542, 0.0f, images[DM_ABOUT_WINDOW], 1.0);
			nvgBeginPath(vg);
			nvgRoundedRect(vg, 0, 0, 1148, 542, 0.0);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
			nvgClosePath(vg);

			nvgFontSize(vg, 25.0f);
			nvgFontFace(vg, "lucon");
			nvgTextAlign(vg, NVG_ALIGN_LEFT);
			nvgFillColor(vg, nvgRGB(255, 153, 255));
			nvgText(vg, 290, 400, version.c_str(), NULL);

			nvgFontSize(vg, 10.0f);
			nvgFontFace(vg, "lucon");
			nvgTextAlign(vg, NVG_ALIGN_LEFT);
			nvgFillColor(vg, nvgRGB(255, 153, 255));
			nvgText(vg, 261, 454, copyw.c_str(), NULL);
		}
	}

	void NVGView::onMoreInfoBtnEnter() {
		this->moreInfoPhase = 1;

	};

	void NVGView::onMoreInfoBtnExit() {
		this->moreInfoPhase = 0;
	};

	void NVGView::onMoreInfoBtnDown() {
		//SHOW BLINK
		Anima ar;
		ar.animName = BUTTON_BLINK_FASTEST;
		ar.controlIdx = MORE_INFO_BUTTON_VIEW_IDX;
		ar.animState = 0;

		RSBView* rsbView = &RViewsMap.at("MORE_INFO_BTN");
		for (auto it = animationReel.begin(); it != animationReel.end(); ++it) {
			if (it->animName == ar.animName && it->controlIdx == ar.controlIdx) {
				animationReel.erase(it);
				break;
			}
		}
		rsbView->blinkIntensity = 0.0;
		rsbView->blinker = 0.0;
		rsbView->blinkerDone = false;

		animationReel.push_back(ar);

		//TOGGLE MAIN WINDOW
		if (moreInfoWindowOpen == false) {
			moreInfoWindowOpen = true;
			for (int i = 0; i < 1000; i++) {
				if (controlMap[i] != nullptr ) {
					controlMap[i]->setMouseEnabled(false);
				}
			}
			if (controlMap[SHUT_WIN_BUTTON_VIEW_IDX] != nullptr) {
				controlMap[SHUT_WIN_BUTTON_VIEW_IDX]->setMouseEnabled(true);
			}
		}
		else {
			moreInfoWindowOpen = false;
			for (int i = 0; i < 1000; i++) {
				if (controlMap[i] != nullptr) {
					controlMap[i]->setMouseEnabled(true);
				}
			}
			if (controlMap[SHUT_WIN_BUTTON_VIEW_IDX] != nullptr) {
				controlMap[SHUT_WIN_BUTTON_VIEW_IDX]->setMouseEnabled(false);
			}
		}
	}


	void NVGView::onShutWinBtnDown() {
		//SHOW BLINK
		Anima ar;
		ar.animName = BUTTON_BLINK_FASTEST;
		ar.controlIdx = SHUT_WIN_BUTTON_VIEW_IDX;
		ar.animState = 0;

		RSBView* rsbView = &RViewsMap.at("SHUT_WIN_BTN");
		int reelSize = static_cast<int>(animationReel.size());
		for (int i = 0; i < reelSize; ) {
			if (animationReel[i].animName == ar.animName && animationReel[i].controlIdx == ar.controlIdx) {
				animationReel.erase(animationReel.begin() + i);
				reelSize--; // Adjust the size
			}
			else {
				i++; // Only increment if no erase
			}
		}
		rsbView->blinkIntensity = 0.0;
		rsbView->blinker = 0.0;
		rsbView->blinkerDone = false;

		animationReel.push_back(ar);

		//TOGGLE MAIN WINDOW
		if (moreInfoWindowOpen == false) {
			moreInfoWindowOpen = true;
			for (int i = 0; i < 1000; i++) {
				if (controlMap[i] != nullptr) {
					controlMap[i]->setMouseEnabled(false);
				}
			}
		}
		else {
			moreInfoWindowOpen = false;
			for (int i = 0; i < 1000; i++) {
				if (controlMap[i] != nullptr) {
					controlMap[i]->setMouseEnabled(true);
				}
			}
			if (controlMap[SHUT_WIN_BUTTON_VIEW_IDX] != nullptr) {
				controlMap[SHUT_WIN_BUTTON_VIEW_IDX]->setMouseEnabled(false);
			}
		}
	}

	void NVGView::setSet(void* parent) {

		

	}


	void NVGView::drawSFML(HWND hwnd, const CRect& updateRect){
			

	}

	PixelFormat* NVGView::getPixelFormat(){
		static PixelFormat pixelFormat;
		pixelFormat.flags = PixelFormat::kMultiSample;
		pixelFormat.samples = 4;
		return &pixelFormat;
	}


	
		
	int NVGView::buttonBlinkFastest(RSBView* rv) {
		if (!rv) {
			return 0;
		}
		else {
			if (rv->blinkerDone == true) {
				return 1;
			}
			else {
				if (rv->blinker == 0.0 && rv->blinkerDone == false) {
					rv->blinkIntensity += 0.133;
					if (rv->blinkIntensity >= 1.0) {
						rv->blinkIntensity = 1.0;
						rv->blinker = 1.0;
					}
				}
				else if (rv->blinker == 1.0 && rv->blinkerDone == false) {
					rv->blinkIntensity -= 0.09;
					if (rv->blinkIntensity <= 0.003) {
						rv->blinkIntensity = 0.0;
						rv->blinker = 0.0;
						rv->blinkerDone = true;
					}
				}
				return 0;
			}
		}

	}




	
	CMouseEventResult NVGView::onMouseDown(CPoint& where, const CButtonState& buttons) {
		
		return kMouseEventHandled; // needed to get the onMouseUp call
	}

	CMouseEventResult NVGView::onMouseUp(CPoint& where, const CButtonState& buttons){
		if (buttons.isLeftButton() && getViewSize().pointInside(where)) {
			int a = 0;
			a++;
		}
			
		return kMouseEventHandled;
	}
	






	char RandyTxt::getRandyChar() {
		char cch = *(randyCharSrc + rand() % 32);
		return cch;
	}


	void RandyTxt::reset(const char* txt, std::string valK, float interv, float incr) {
		if (strcmp(valK.c_str(), valueKey.c_str()) == 0 || shouldInc == true) {
			if (shouldDecay == true) {
				valueKey = valK;
				shouldInc = true;
				textDisplayTimerNextIntrv = interv;
				textDisplayTimerNextIncr = incr;
				selectedCtrlTarg = txt;
				valLen = static_cast<int>(selectedCtrlTarg.size());
				textDisplayDecay = 1.0f;
				textDisplayTimer = 0.0f;
				textDisplayIdx = 0;
				textDisplayRevIdx = 0;
				procRevTimer = 0.0f;
				shouldDecayTimer = 0.0f;
			}
			else {
				selectedCtrlTarg = txt;
			}
		}

		else {
			valueKey = valK;
			shouldInc = true;
			textDisplayTimerNextIntrv = interv;
			textDisplayTimerNextIncr = incr;
			selectedCtrlTarg = txt;
			valLen = static_cast<int>(selectedCtrlTarg.size());
			textDisplayDecay = 1.0f;
			textDisplayTimer = 0.0f;
			textDisplayIdx = 0;
			textDisplayRevIdx = 0;
			procRevTimer = 0.0f;
			shouldDecayTimer = 0.0f;
		}
	}


	void RandyTxt::processText() {

		if (shouldInc == true) {
			textDisplayTimer += 0.01f;
			if (textDisplayTimer >= textDisplayTimerNextIntrv) {
				textDisplayTimerNextIntrv = textDisplayTimer + textDisplayTimerNextIncr;
				textDisplayIdx += 1;
			}
			valLen = static_cast<int>(selectedCtrlTarg.size());
			selectedCtrlTargBuilder = "";
			if (textDisplayIdx < valLen) {
				for (int i = 0; i < valLen; i++) {
					selectedCtrlTargBuilder += getRandyChar();
				}
				for (int i = 0; i < textDisplayIdx; i++) {
					selectedCtrlTargBuilder.at(i) = selectedCtrlTarg.at(i);
				}
			}
			else {
				selectedCtrlTargBuilder = selectedCtrlTarg;
				shouldInc = false;
			}
		}
		else {
			selectedCtrlTargBuilder = selectedCtrlTarg;
			shouldDecayTimer += 0.01;
			if (shouldDecayTimer < 0.75f) {
				textDisplayRevIdx = 0;
			}
			else {
				shouldDecay = true;
				if (textDisplayDecay >= 0.064f) {
					textDisplayDecay -= 0.064f;
				}
				else {
					textDisplayDecay = 0.0f;
				}
				textDisplayTimer += 0.02f;
				if (textDisplayTimer >= textDisplayTimerNextIntrv) {
					textDisplayTimerNextIntrv = textDisplayTimer + textDisplayTimerNextIncr;
					textDisplayRevIdx += 1;
				}
				valLen = static_cast<int>(selectedCtrlTarg.size());
				selectedCtrlTargBuilder = selectedCtrlTarg;
				if (textDisplayRevIdx < valLen) {
					for (int i = 0; i < textDisplayRevIdx; i++) {
						selectedCtrlTarg.at(i) = getRandyChar();
					}
				}
				else {
					for (int i = 0; i < valLen; i++) {
						selectedCtrlTarg.at(i) = getRandyChar();
					}
				}

			}

		}

	}

	void RandyTxt::processTextRev() {
		procRevTimer += 0.01f;
		if (procRevTimer > 1.2f) {
			textDisplayTimer += 0.01f;
			if (textDisplayTimer >= textDisplayTimerNextIntrv) {
				textDisplayTimerNextIntrv = textDisplayTimer + textDisplayTimerNextIncr;
				textDisplayRevIdx += 1;
			}
			valLen = static_cast<int>(selectedCtrlTarg.size());
			selectedCtrlTargBuilder = selectedCtrlTarg;
			if (textDisplayRevIdx < valLen) {
				for (int i = 0; i < textDisplayRevIdx; i++) {
					selectedCtrlTarg.at(i) = getRandyChar();
				}
			}
			else {
				for (int i = 0; i < valLen; i++) {
					selectedCtrlTarg.at(i) = getRandyChar();
				}
			}
		}

	}


}
}
}