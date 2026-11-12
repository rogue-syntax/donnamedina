#pragma once
//#include "REvent.h"
//#include "REventMixin.h"
#include "REventable.h"


class REventBinder {
public:
	
	REventBinder(REventable* objContext, void (REventable::*handler)(const REvent& e));
	
	void handler(const REvent& e);

	REventable* boundObjContext;
	void(REventable::*boundHandler)(const REvent&);
	REventBinder() {};
	~REventBinder() {};
};


class RDataBinder {
public:

	RDataBinder(RDataConsumer* objContext, void (RDataConsumer::*handler)(RData& e), std::string dataNameRef);
	std::string dataName;
	void handler(RData& e);

	RDataConsumer* boundObjContext;
	void(RDataConsumer::*boundHandler)(RData&);
	RDataBinder() {};
	~RDataBinder() {};
};


class RFunctionBinder {
public:

	RFunctionBinder(RDataConsumer* objContext, void (RDataConsumer::*handler)(), std::string funcNameRef);
	std::string funcName;
	void handler();

	RDataConsumer* boundObjContext;
	void(RDataConsumer::*boundHandler)();
	RFunctionBinder() {};
	~RFunctionBinder() {};
};
