#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/controls/icontrollistener.h"

using namespace VSTGUI;


	class VSwitchListener : public IControlListener {
	public:

		void valueChanged(CControl* pControl) override;


	};