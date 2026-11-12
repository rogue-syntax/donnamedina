
//#pragma once

#include "NVGView.h"
#include "vstgui/uidescription/delegationcontroller.h"

//typedef struct NVGcontext;

using namespace VSTGUI;
namespace Steinberg {
namespace Vst {
namespace rs {


class NVGViewController : public DelegationController{
public:
	
	NVGView* nvgView = nullptr;
	bool isCreated;
	
	NVGViewController(IController* iController, BaseController* dx10ptr): DelegationController(iController) {
		nvgView = new NVGView(CRect(0, 0, 1148, 542), dx10ptr);
		if (iController == nullptr) {
			isCreated = false;
		}
		else {
			isCreated = true;
		};

	};
	
	~NVGViewController() {
		//delete nvgView;
	}
	

	IControlListener* getControlListener(UTF8StringPtr name) override;

	CView* createView(const UIAttributes& attributes, const IUIDescription* description) override;


	//NVGcontext* vg;

};

};
};
};