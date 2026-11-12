#ifndef __rsogv__
#define __rsogv__

#pragma once
#include <string>
#include "vstgui4/vstgui/lib/crect.h"
#include "REvent.h"

using namespace VSTGUI;

class RSOGView : public REvent {
public:
	RSOGView(std::string eventNameIn = "") : REvent(eventNameIn),
		viewId(0),
		viewName(""),
		viewType(""),
		drawRect(0, 0, 0, 0),
		value(0.0),
		mouseActive(false),
		displayActive(false),
		x(0.0f),
		y(0.0f),
		z(0.0f)
	{
	};

	int viewId;
	std::string viewName;
	std::string viewType;
	CRect drawRect;
	double value;
	double min = 0.0;
	double max = 0.0;
	bool mouseActive;
	bool displayActive;
	float x;
	float y;
	float z;

};

class RSBView : public RData {
public:
	RSBView(std::string eventNameIn = "") : RData(eventNameIn),
		type(0),
		viewId(0),
		strokeWidth1(0.0f),
		strokeWidth2(0.0f),
		viewName(eventNameIn), // Initialize with the provided name or empty
		viewType(0),
		overlayType(0),
		overlayHeight(0),
		overlayWidth(0),
		overlayX(0),
		overlayY(0),
		drawRect(0, 0, 0, 0),
		value(0.0f),
		mouseActive(false),
		displayActive(false),
		x(0.0f),
		y(0.0f),
		z(0.0f),
		alpha(0.0f),
		blinkIntensity(0.0f),
		blinker(0.0f),
		blinkerDone(false),
		baseX(0.0f),
		baseY(0.0f),
		baseZ(0.0f),
		lowerRightX(0.0f),
		lowerRightY(0.0f),
		lowerLeftX(0.0f),
		lowerLeftY(0.0f),
		upperLeftX1(0.0f),
		upperLeftY1(0.0f),
		upperLeftX2(0.0f),
		upperLeftY2(0.0f),
		height(0.0f),
		width(0.0f),
		state(""),
		nameLen(static_cast<int>(eventNameIn.length())), // Initialize based on the provided name
		loc(0, 0),
		RSBType(0)
	{
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
	float min = 0.0f;
	float max = 0.0f;
	bool mouseActive;
	bool displayActive;
	//x and y are animation destinations
	float x;
	float y;
	float z;
	float alpha;
	float blinkIntensity;
	float blinker;
	bool blinkerDone;
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
	int RSBType;
};


#endif