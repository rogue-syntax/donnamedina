#pragma once
#include "REvents.h"



void REvents::createREvent(const std::string& eventNameRef, const REvent& newEvent) {
	std::string eventName = eventNameRef;
	//REvent newEvent = REvent(eventName);
	std::pair<std::string, REvent> insertPair = std::pair<std::string,REvent>(eventName, newEvent);
	REventsMap.insert(insertPair);
}

 
void REvents::createREventBinder(const std::string& eventNameRef, REventable* objContext, void (REventable::*handler)(const REvent& e)) {
	REventBinder evBinder(objContext, handler);
	if (boundBinders.find(eventNameRef) == boundBinders.end()) {
		boundBinders[eventNameRef] = std::vector<REventBinder>();
	}
	boundBinders[eventNameRef].push_back(evBinder);
}


void REvents::dispatchEvent(REvent* rEv) {
	REventQueue.push_back(rEv);
}

void REvents::dispatchEvent(std::string eventNameIn) {
	REvent* revp = &REventsMap.at(eventNameIn);
	REventQueue.push_back( revp );
}


void REvents::execQueue() {
	for (REvent* rEvent : REventQueue) {
		for (REventBinder& rEvBinder : boundBinders[(*rEvent).eventName]) {
			rEvBinder.handler((*rEvent));
		}
	}
	REventQueue.clear();
}




//RDATA HANDLER LIB


void REvents::createRData(std::string dataNameRef, RData newData) {
	//std::string dataName;
	//dataName = dataNameRef;
	//newData.eventName = dataNameRef;
	RData rd1(dataNameRef);
	//std::string str = newData.eventName;
	std::pair<std::string, RData> insertPair = std::pair<std::string, RData>(dataNameRef, newData);
	//RDataMap[dataNameRef] = rd1;
	RDataMap.insert(insertPair);
}


void REvents::createRDataBinder(const std::string& handleNameRef, const std::string& dataNameRef, RDataConsumer* objContext, void (RDataConsumer::*handler)(RData& e)) {
	RDataBinder evBinder(objContext, handler, dataNameRef);
	if (boundHandlers.find(handleNameRef) == boundHandlers.end()) {
		boundHandlers[handleNameRef] = evBinder;
	}else {
		boundHandlers.erase(handleNameRef);
		boundHandlers[handleNameRef] = evBinder;
	}
	
}

void REvents::execDataHandler(std::string handleNameRef) {
	RData rd = RDataMap.at(boundHandlers[handleNameRef].dataName);

	boundHandlers[handleNameRef].handler( rd );//not calling the handler with data correctly
}




//RFunction
void REvents::newFunction(const std::string& funcNameRef, RDataConsumer* objContext, void (RDataConsumer::*handler)()) {
	RFunctionBinder evBinder(objContext, handler, funcNameRef);
	if (functionMap.find(funcNameRef) == functionMap.end()) {
		functionMap[funcNameRef] = evBinder;
	}
	else {
		functionMap.erase(funcNameRef);
		functionMap[funcNameRef] = evBinder;
	}

}

void REvents::callFunction(std::string fName) {
	functionMap[fName].handler();
}



