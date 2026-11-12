#pragma once

//#include "nanovg.h"
//#define NANOVG_GL_IMPLEMENTATION
//#define NANOVG_GL3_IMPLEMENTATION
//#include "nanovg_gl.h"
//#include "nanovg_gl_utils.h"


using namespace VSTGUI;
namespace Steinberg {
namespace Vst {
namespace rs {


class NVGClass {
public:
	NVGcontext* vg;
	NVGClass* ptr;
	NVGClass() {
		ptr = new NVGClass();
	}
	NVGClass* p() {
		return ptr;
	};

};

}
}
}