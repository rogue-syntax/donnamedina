#pragma once
#include "REventBinder.h"

//REVENT BINDER
REventBinder::REventBinder(REventable* objContext, void (REventable::*handler)(const REvent& e) ) {
	boundObjContext = objContext;
	boundHandler = handler;
}
//set the handler
void REventBinder::handler(const REvent& e) {
	(boundObjContext->*boundHandler)(e);

}


//RDATAINDER
RDataBinder::RDataBinder(RDataConsumer* objContext, void (RDataConsumer::*handler)(RData& e), std::string dataNameRef) {
	dataName = dataNameRef;
	boundObjContext = objContext;
	boundHandler = handler;
}
//set the handler
void RDataBinder::handler(RData& e) {
	(boundObjContext->*boundHandler)(e);

}



//RFUNCTION
RFunctionBinder::RFunctionBinder(RDataConsumer* objContext, void (RDataConsumer::*handler)(), std::string funcNameRef) {
	funcName = funcNameRef;
	boundObjContext = objContext;
	boundHandler = handler;
}
//set the handler
void RFunctionBinder::handler() {
	(boundObjContext->*boundHandler)();
}