#pragma once
//#include "REvent.h"
//#include "REventMixin.h"
#include "./REventable.h"


class REventBinder {
public:
	
	REventBinder(REventable* objContext, void (REventable::*handler)(const REvent& e)) {
		boundObjContext = objContext;
		boundHandler = handler;
	}
	
	void handler(const REvent& e) {
		(boundObjContext->*boundHandler)(e);

	}

	REventable* boundObjContext;
	void(REventable::*boundHandler)(const REvent&);
	REventBinder() {};
	~REventBinder() {};
};


class RDataBinder {
public:

	RDataBinder(RDataConsumer* objContext, void (RDataConsumer::*handler)(RData& e), std::string dataNameRef) {
		dataName = dataNameRef;
		boundObjContext = objContext;
		boundHandler = handler;
	}
	std::string dataName;
	void handler(RData& e) {
		(boundObjContext->*boundHandler)(e);
	}

	RDataConsumer* boundObjContext;
	void(RDataConsumer::*boundHandler)(RData&);
	RDataBinder() {};
	~RDataBinder() {};
};

class RVoiceBinder {
public:

	RVoiceBinder(RDataConsumer* objContext, void (RDataConsumer::*handler)(RVoice& e), int idIn) {
		id = idIn;
		boundObjContext = objContext;
		boundHandler = handler;
	}
	int id;
	void handler(RVoice& e) {
		(boundObjContext->*boundHandler)(e);
	}

	RDataConsumer* boundObjContext;
	void(RDataConsumer::*boundHandler)(RVoice&);
	RVoiceBinder() {};
	~RVoiceBinder() {};
};


class RFunctionBinder {
public:

	RFunctionBinder(RDataConsumer* objContext, void (RDataConsumer::*handler)(), std::string funcNameRef) {
		funcName = funcNameRef;
		boundObjContext = objContext;
		boundHandler = handler;
	}
	std::string funcName;
	void handler() {
		(boundObjContext->*boundHandler)();
	}

	void updateContext(RDataConsumer* objContext) {
		boundObjContext = objContext;
	}

	void reBind(RDataConsumer* objContext, void (RDataConsumer::*handler)(), std::string funcNameRef) {
		funcName = funcNameRef;
		boundObjContext = objContext;
		boundHandler = handler;
	}

	RDataConsumer* boundObjContext;
	void(RDataConsumer::*boundHandler)();
	RFunctionBinder() {};
	~RFunctionBinder() {};
};

