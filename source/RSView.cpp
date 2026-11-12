#pragma once

#include "RSView.h"
#include <functional>
#include <list>
#include "../base/source/updatehandler.h"
#include "NVGView.h"

namespace Steinberg {
namespace Vst {
namespace rs {
	
	
	RSView::~RSView() {
	}

	//VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer;
	//std::atomic<uint32_t> users{ 0 };
	

	tresult PLUGIN_API RSView::attached(void* parent, FIDString type) {
		VST3Editor::attached(parent, type);
		//void* parent is window handle HWND__*
		return kResultTrue;
	};

	
	bool RSView::open(void* parent, const PlatformType& platformType) {
		
		VST3Editor::open(parent, platformType);
		
		return true;
		
	}

	
	void RSView::close(){
		VST3Editor::close();
	}

	tresult PLUGIN_API RSView::canResize(){
		return Steinberg::kResultFalse;
	}


	
	
	//void RSView::onClicked() {
//
	//}
	

}
}
}