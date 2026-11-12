#ifndef __EVENTS__
#define __EVENTS__
#pragma once
#include <vector>
#include <array>
#include "REventBinder.h"
#include "map.h"


class REvents {
public:
	REvents() {};
	//REvents();


	//REVENT////////////////////////////////
	//////////////////////////////////////////
	Map::hash<std::string, REvent> REventsMap;
	Map::hash< std::string, std::vector<REventBinder> >boundBinders;
	std::vector<REvent*> REventQueue;


	void createREvent(const std::string& eventNameRef, const REvent& newEvent) {
		std::string eventName = eventNameRef;
		//REvent newEvent = REvent(eventName);
		std::pair<std::string, REvent> insertPair = std::pair<std::string, REvent>(eventName, newEvent);
		REventsMap.insert(insertPair);
	}


	void createREventBinder(const std::string& eventNameRef, REventable* objContext, void (REventable::*handler)(const REvent& e)) {
		REventBinder evBinder(objContext, handler);
		if (boundBinders.find(eventNameRef) == boundBinders.end()) {
			boundBinders[eventNameRef] = std::vector<REventBinder>();
		}
		boundBinders[eventNameRef].push_back(evBinder);
	}


	void dispatchEvent(REvent* rEv) {
		REventQueue.push_back(rEv);
	}

	void dispatchEvent(std::string eventNameIn) {
		REvent* revp = &REventsMap.at(eventNameIn);
		REventQueue.push_back(revp);
	}


	void execQueue() {
		for (REvent* rEvent : REventQueue) {
			for (REventBinder& rEvBinder : boundBinders[(*rEvent).eventName]) {
				rEvBinder.handler((*rEvent));
			}
		}
		REventQueue.clear();
	}

	//RDATA//////////////////////////////////
	//////////////////////////////////////////
	Map::hash<std::string, RData> RDataMap;
	Map::hash< std::string, RDataBinder >boundHandlers;

	void createRData(std::string dataNameRef, RData newData) {
		RData rd1(dataNameRef);//wot??
		std::pair<std::string, RData> insertPair = std::pair<std::string, RData>(dataNameRef, newData);
		RDataMap.insert(insertPair);
	}


	void createRDataBinder(const std::string& handleNameRef, const std::string& dataNameRef, RDataConsumer* objContext, void (RDataConsumer::*handler)(RData& e)) {
		RDataBinder evBinder(objContext, handler, dataNameRef);
		if (boundHandlers.find(handleNameRef) == boundHandlers.end()) {
			boundHandlers[handleNameRef] = evBinder;
		}
		else {
			boundHandlers.erase(handleNameRef);
			boundHandlers[handleNameRef] = evBinder;
		}

	}

	void execDataHandler(std::string handleNameRef) {
		RData rd = RDataMap.at(boundHandlers[handleNameRef].dataName);

		boundHandlers[handleNameRef].handler(rd);
	}



	//RFunction////////////////////////
	////////////////////////////////////
	Map::hash< std::string, RFunctionBinder >functionMap;

	void newFunction(const std::string& funcNameRef, RDataConsumer* objContext, void (RDataConsumer::*handler)()) {
		RFunctionBinder evBinder(objContext, handler, funcNameRef);
		if (functionMap.find(funcNameRef) == functionMap.end()) {
			functionMap[funcNameRef] = evBinder;
		}
		else {
			functionMap.erase(funcNameRef);
			functionMap[funcNameRef] = evBinder;
		}

	}

	void updateContext(std::string fName, RDataConsumer* objContext) {
		functionMap[fName].updateContext(objContext);
	}

	void reBind(const std::string& funcNameRef, RDataConsumer* objContext, void (RDataConsumer::*handler)()) {
		functionMap[funcNameRef].reBind(objContext, handler, funcNameRef);
	}

	void callFunction(std::string fName) {
		functionMap[fName].handler();
	}





	
	//RVOICE////////////////////////////////
	//////////////////////////////////////////
	

	//std::unordered_map<int, RVoiceBinder> boundProcessors;
	//std::unordered_map<int, std::vector<RVoiceBinder*> > boundProcessors;
	std::map<int, std::vector<RVoiceBinder> > boundProcessors;

	//std::unordered_map< int, std::vector<REventBinder> >boundBinders;
	std::array<RVoice, PARTVOICES> RVoiceMap;
	//std::array<RVoice*, PARTVOICES> RVoiceEventQueue;
	std::vector<int> RVoiceEventQueue;
	std::vector<int> ReVoiceEventQueue;

	void createRVoice(int inID, RVoice& newData) {
		RVoiceMap[inID] = newData;
	}

	void createRVoiceBinder(const std::string& handleNameRef, const int& idIn, RDataConsumer* objContext, void (RDataConsumer::*handler)(RVoice& e)) {
		RVoiceBinder evBinder(objContext, handler, idIn);
		if (boundProcessors.find(idIn) == boundProcessors.end()) {

			//boundProcessors[idIn] = evBinder;
			boundProcessors[idIn] = std::vector<RVoiceBinder>();
		}
			boundProcessors[idIn].push_back(evBinder);
		//}else {
		//	boundProcessors.erase(idIn);
		//	boundProcessors[idIn] = evBinder;
		//}
	}

	void execVoiceProcess(int id) {
		//RVoiceEventQueue[id]
		//boundProcessors[id].handler(RVoiceMap[id]);
	}

	void dispatchVoiceEvent(RVoice* rEv) {
		RVoiceEventQueue.push_back(rEv->id);
	}

	void dispatchVoiceEvent(int veID) {
		RVoiceEventQueue.push_back(veID);
		//RVoiceEventQueue[veID] = &RVoiceMap[veID];
	}

	void reDispatchVoiceEvent(int veID) {
		ReVoiceEventQueue.push_back(veID);

	}

	void execVoiceQueue() {
		for (int& id : RVoiceEventQueue) {
			for (RVoiceBinder& rEvBinder : boundProcessors[id]) { 
			
				rEvBinder.handler(RVoiceMap[id]);

			}
		}
		//RVoiceEventQueue.clear();
		//for (int& id : ReVoiceEventQueue) {
		//	RVoiceEventQueue.push_back(id);
		////}
		//ReVoiceEventQueue.clear();
	}

	

};

#endif