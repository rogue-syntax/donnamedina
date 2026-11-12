#pragma once

//#include "SFMLView.h"

#include "vstgui4/vstgui/vstgui.h"
#include "vstgui4/vstgui/plugin-bindings/vst3editor.h"
//#include "MainView.h"
//#include "glIncludes.h"

using namespace VSTGUI;
namespace Steinberg {
namespace Vst {
namespace rs {

	//is an i_control_listner
	class RSView : public VST3Editor {
		//class RSView : public VSTGUI::CView {
	public:

		RSView(EditController* controller
			, UTF8StringPtr templateName
			, UTF8StringPtr xmlFile
			, const CRect& size) : VST3Editor(controller, templateName, xmlFile) {};
		
		~RSView();
		
		bool PLUGIN_API open(void* parent, const PlatformType& platformType = kDefaultNative) SMTG_OVERRIDE;

		int counter = 0;

		void PLUGIN_API close();
		
		tresult PLUGIN_API attached(void* parent, FIDString type) SMTG_OVERRIDE;

		void onClicked();
	
		tresult PLUGIN_API canResize() override;


	};
}
}
}
