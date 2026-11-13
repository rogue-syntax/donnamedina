
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
	BaseController* owner = nullptr;  // B-29: told when this controller is destroyed

	NVGViewController(IController* iController, BaseController* dx10ptr): DelegationController(iController), owner(dx10ptr) {
		nvgView = new NVGView(CRect(0, 0, ::editorLayout::kEditorWidth, ::editorLayout::kEditorHeight), dx10ptr);
		if (iController == nullptr) {
			isCreated = false;
		}
		else {
			isCreated = true;
		};

	};
	
	~NVGViewController() {
		//delete nvgView;
		// B-29: VSTGUI deletes this sub-controller when the editor closes; the owning controller
		// kept a raw pointer to it (dangling after close). Let it clear that pointer.
		if (owner)
			owner->onNVGViewControllerDestroyed(this);
	}
	

	IControlListener* getControlListener(UTF8StringPtr name) override;

	CView* createView(const UIAttributes& attributes, const IUIDescription* description) override;


	//NVGcontext* vg;

};

};
};
};