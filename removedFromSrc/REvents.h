#pragma once
#include <vector>
#include "REventBinder.h"
#include "map.h"


class REvents {
public:
	REvents() {};


	// 1: 
	/*CreateREvent: Creates newEvent Object.
		1: Event objects need to be created in the REvents map.
		Takes a const refrence to a name string from the calling scope.
		Creates a std::string and copies the input string value here.
		Creates an Event Object here.
		Both should be stored here and refrenced elsewhere.
		Creates a std::pair, which holds refrences to the nameString and event object.
		Inserts pair into the REventsMap.I belive the hash map is using const refrences of the pair items.Hash
		map lib is ska.h by Malte Skarupke;
	*/ 
	void createREvent(const std::string& eventNameRef, const REvent& newEvent);
	Map::hash<std::string, REvent> REventsMap;



	// 2: 
	/*CreateREventBinder: 
		Creates a new REventBinder. 
		Takes a name string refrence and maps a std::vector of EventBinders if map pair doesnt allready exist.
		Appends new REventBinder to the vector: "eventName" : [ {REventBinder}, {REventBinder}, ... ]
		Takes a pointer to an object of REventable type, and a handler member of said REventable object type that takes an argument of type const REvent&.
		REventBinder caches pointers to the Context object and the Context::handler method of that type.
		Each REventBinder has a REventBinder::handler method that calls the Context::handler method.
		When the bound REvent queue fires, the REventBinder::handler calls Context::handler with a refrence to the REvent as the argument;
		It looks like: void (REventable::*handler)(const REvent& e);
	*/
	void createREventBinder(const std::string& eventNameRef, REventable* classContext, void (REventable::*handler)(const REvent& e));
	Map::hash< std::string, std::vector<REventBinder> >boundBinders;


	// 3:
	/* dispatchEvents:
		When an event is dispatched , an REvent refrence is pulled by string name from the REventsMap and added to the REventsQueue.
		REvent can be refrenced directy or by name string.
	*/
	void dispatchEvent(REvent* event);
	void dispatchEvent(std::string eventName);
	std::vector<REvent*> REventQueue;


	// 4:
	/* execQueue:
		When its time to execute the REvents that have been dispatched, execQueue iterates through the REventQueue.
		For each REvent refrence in the queue, it gets refrence to the std::vector of REventBinders mapped under the REvent::eventName of the REvent
		I.E. "eventName" : [ {REventBinder}, {REventBinder}, ... ]
		Each REventBinder::handler is called with a const refrence to the dispatched event as the argument.
		The REventQueue is cleared -- for regular non-persistant events in a thread loop, etc.
	*/
	void execQueue();


	


	void createRData(std::string dataNameRef, RData newData);
	Map::hash<std::string, RData> RDataMap;
	
	void createRDataBinder(const std::string& handlerNameRef, const std::string& dataNameRef, RDataConsumer* classContext, void (RDataConsumer::*handler)(RData& e));
	Map::hash< std::string, RDataBinder >boundHandlers;

	void execDataHandler(std::string handlerName);




	//RFUNCTION
	void newFunction(const std::string& funcNameRef, RDataConsumer* classContext, void (RDataConsumer::*handler)());
	Map::hash< std::string, RFunctionBinder >functionMap;
	void callFunction(std::string fName);


};