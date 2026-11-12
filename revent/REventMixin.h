#pragma once
#include <string>
#include <vector>
//#include "RSOGView.h"

	class REventMixin {
	public:
		template<typename T> 
		using REvent = T;
	};

	class RDataMixin {
	public:
		template<typename T>
		using RData = T;
	};


