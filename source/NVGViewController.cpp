
#include "NVGViewController.h"
//#include "mdaDX10Controller.h"
namespace Steinberg {
	namespace Vst {
		namespace rs {
			IControlListener*  NVGViewController::getControlListener(UTF8StringPtr name) {
				if (strcmp(name, "threaded") == 0) {
					return this;
				}
				return controller->getControlListener(name);
			}

			CView*  NVGViewController::createView(const UIAttributes& attributes, const IUIDescription* description) {
				const std::string* name = attributes.getAttributeValue("custom-view-name");
				if (name && *name == "nvgView") {
					return nvgView;
				}
				return controller->createView(attributes, description);
			}
		}
	}
}
