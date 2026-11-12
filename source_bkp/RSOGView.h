#ifndef __rsogv__
#define __rsogv__

#pragma once
#include <string>
#include "vstgui4/vstgui/lib/crect.h"
#include "../SFML-2.5.1-64/include/SFML/revent/REvent.h"

using namespace VSTGUI;

class RSOGView : public REvent{
public:
	RSOGView(std::string eventNameIn) : REvent(eventNameIn) {};

	int viewId;
	std::string viewName;
	std::string viewType;
	CRect drawRect;
	double value;
	double min;
	double max;
	bool mouseActive;
	bool displayActive;
	float x;
	float y;
	float z;

};

class RSBView : public RData {
public:
	RSBView(std::string eventNameIn) : RData(eventNameIn) {
		nameLen = static_cast<int>(eventNameIn.length());
		viewName = eventNameIn;
	};
	int type;
	int viewId;
	float strokeWidth1;
	float strokeWidth2;
	std::string viewName;
	int viewType;
	int overlayType;
	int overlayHeight;
	int overlayWidth;
	int overlayX;
	int overlayY;
	CRect drawRect;
	float value;
	float min;
	float max;
	bool mouseActive;
	bool displayActive;
	//x and y are animation destinations
	float x;
	float y;
	float z;
	float alpha;
	float blinkIntensity = 0.0;
	float blinker = 0.0;
	bool blinkerDone = false;
	float baseX;
	float baseY;
	float baseZ;
	float lowerRightX;
	float lowerRightY;
	float lowerLeftX;
	float lowerLeftY;
	float upperLeftX1;
	float upperLeftY1;
	float upperLeftX2;
	float upperLeftY2;
	float height;
	float width;
	std::string state;
	int nameLen;//for sizing name text displays
	CPoint loc;
	int RSBType = 0;
};


#endif


