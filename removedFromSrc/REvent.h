#pragma once
#include <string>

class REvent {
public:

	std::string eventName;
	REvent(std::string eventNameIn) : eventName(eventNameIn) {};
	
};

class RData {
public:
	//RData() {};
	std::string eventName;
	RData(std::string eventNameIn) : eventName(eventNameIn) {};

};