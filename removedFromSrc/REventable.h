#pragma once
#include "REvent.h"


class REventable {
public:
	//void onDrawNvg(const REventMixin::Ev<RSOGView>& ev) {};
	virtual void onDrawOsc(const REvent& ev) {};
	virtual void onDrawNvg(const REvent& ev) {};
};


class RDataConsumer {
public:
	//RFunction
	virtual void setupRKnob() {};
	virtual void setupRButton() {};

	//RData
	virtual void drawBkg(RData& obj) {};
	virtual void drawKnob(RData& obj) {};
	virtual void drawButton(RData& obj) {};
	virtual void drawBlank(RData& obj) {};

};
