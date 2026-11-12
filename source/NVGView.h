

#pragma once



//#include "SFClass.h"
//#include "mdaDX10Controller.h"
#include "mdaBaseController.h"
#include "constants.h"
#include <time.h> 
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/controls/icontrollistener.h"
#include <string>
#include "base/source/fobject.h"
//#include "vstgui/uidescription/delegationcontroller.h"
#include "vstgui/uidescription/uiattributes.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "pluginterfaces/base/ustring.h"
#include <windows.h>
//#include "REvents.h"
//#include "REventable.h"
#include "REvents.h"
#include "REventable.h"
#include "GLShader.h"

//#include "glIncludes.h"

//#include "VSwitchListener.h"


////instead of gl includes
#include <GL/glew.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"
////

#include "map.h"
#include "RSOGView.h"

#include <functional>


using namespace VSTGUI;
namespace Steinberg {
namespace Vst {
namespace rs {


	class MoreInfoButton : public CView {
	public:

		CGraphicsPath* path;

		using MouseEnterCallback = std::function<void()>;
		MouseEnterCallback onMouseEnterCallback;

		using MouseExitCallback = std::function<void()>;
		MouseExitCallback onMouseExitCallback;

		using MouseDownCallback = std::function<void()>;
		MouseDownCallback onMouseDownCallback;

		RSBView rsbView;

		MoreInfoButton(const CRect& size, MouseEnterCallback onEnter, MouseExitCallback onExit, MouseExitCallback onDown) : CView(size)
			, onMouseEnterCallback(onEnter)
			, onMouseExitCallback(onExit)
			, onMouseDownCallback(onDown)
			, rsbView("MORE_INFO_BUTTON") 
		{


			setWantsFocus(true);


		};

		CPoint pathOutline[8] = {
			CPoint(965, 314),
			CPoint(1020.5, 314),
			CPoint(1026.5, 320),
			CPoint(1112, 320),
			CPoint(1112, 375),
			CPoint(1026.5, 375),
			CPoint(1020.5, 369),
			CPoint(965, 369) };



		void initializePath(CDrawContext* context) {
			if (!path) {
				path->beginSubpath(pathOutline[0]);
				path->addLine(pathOutline[1]);
				path->addLine(pathOutline[2]);
				path->addLine(pathOutline[3]);
				path->addLine(pathOutline[4]);
				path->addLine(pathOutline[5]);
				path->addLine(pathOutline[6]);
				path->addLine(pathOutline[7]);
				//path->addLine(CPoint(963, 312));
				path->closeSubpath();
			}
		}

		void draw(CDrawContext* context) override {
			initializePath(context);
			CView::draw(context);
		}

		CMouseEventResult onMouseEntered(CPoint& where, const CButtonState& buttons) override;
		CMouseEventResult onMouseExited(CPoint& where, const CButtonState& buttons) override {
			if (onMouseExitCallback) {
				onMouseExitCallback();
			}
			return kMouseEventHandled;
		}
		CMouseEventResult onMouseDown(CPoint& where, const CButtonState& buttons) override {
			if (onMouseDownCallback) {
				onMouseDownCallback();
			}
			return kMouseEventHandled;
		}
	};

	inline CMouseEventResult MoreInfoButton::onMouseEntered(CPoint& where, const CButtonState& buttons) {
		if (onMouseEnterCallback) {
			onMouseEnterCallback();
		}
		return kMouseEventHandled;
	}

const int ANIMBANKSIZE = 20;
/*
class AClass {
public:
	AClass() {};
	~AClass() {};

};
*/

class GLPoint {
public:
	GLPoint() {};
	~GLPoint() {};

	float x;
	float y;
	float z;

	void define(float X, float Y, float Z) {
		x = X;
		y = Y;
		z = Z;
	}
};

class GLTri {
public:
	GLTri() {};
	~GLTri() {};

	GLPoint a;
	GLPoint b;
	GLPoint c;
};

class GLQuad {
public:
	GLQuad() {};
	~GLQuad() {};

	GLPoint a;
	GLPoint b;
	GLPoint c;
	GLPoint d;
};

class GLPyramid {
public:
	GLPyramid() {
		p1.define(-1.0f, -1.f, -1.f);
		p2.define(1.f, -1.f, -1.f);
		p3.define(1.0f, -1.0f, 1.0f);
		p4.define(-1.f, -1.f, 1.f);
		p5.define(0.0f, 1.0f, 0.0f);
		scaler.define(0.5f, 0.5f, 0.5f);
		transl.define(0.0f, 0.0f, 0.0f);

	};
	~GLPyramid() {};

	GLTri a;
	GLTri b;
	GLTri c;
	GLQuad d;

	GLPoint p1;
	GLPoint p2;
	GLPoint p3;
	GLPoint p4;
	GLPoint p5;

	float rotX = 0.0f;
	float rotY = 0.0f;
	float rotZ = 0.0f;

	float x_cs  = 0.0f;
	float y_cs = 0.0f;

	GLPoint scaler;
	GLPoint transl;

	void define(GLPoint P1, GLPoint P2, GLPoint P3, GLPoint P4, GLPoint P5) {
		
		p1.x = P1.x;
		p1.y = P1.y;
		p1.z = P1.z;

		p2.x = P2.x;
		p2.y = P2.y;
		p2.z = P2.z;

		p3.x = P3.x;
		p3.y = P3.y;
		p3.z = P3.z;

		p4.x = P4.x;
		p4.y = P4.y;
		p4.z = P4.z;

		p5.x = P5.x;
		p5.y = P5.y;
		p5.z = P5.z;

	}

	void scale(GLPoint SCALER) {
		scaler.x = SCALER.x;
		scaler.y = SCALER.y;
		scaler.z = SCALER.z;

	}

	void trans(GLPoint SCALER) {
		transl.x = SCALER.x;
		transl.y = SCALER.y;
		transl.z = SCALER.z;

	}

	void trans2D(float X, float Y, float W, float H) {
		transl.x = 2.0f * (X + 0.5f) / W - 1.0f;
		transl.y = 2.0f * (Y + 0.5f) / H - 1.0f;

	}


	void draw() {
		//trans2D(x_cs, y_cs, 1148, 542);
		glEnable(GL_MULTISAMPLE);
		glMatrixMode(GL_PROJECTION);//?
		glLoadIdentity();//?
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glTranslatef(transl.x, transl.y, transl.z);

		glScaled(scaler.x, scaler.y, scaler.z);

		glRotated(rotX, 1, 0, 0);
		glRotated(rotY, 0, 1, 0);
		glRotated(rotZ, 0, 0, 1);

		glBegin(GL_TRIANGLES);

		glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
		glVertex3f(p5.x, p5.y, p5.z);
		glVertex3f(p1.x, p1.y, p1.z);
		glVertex3f(p3.x, p3.y, p3.z);

		glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
		glVertex3f(p5.x, p5.y, p5.z);
		glVertex3f(p3.x, p3.y, p3.z);
		glVertex3f(p2.x, p2.y, p2.z);

		glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
		glVertex3f(p5.x, p5.y, p5.z);
		glVertex3f(p2.x, p2.y, p2.z);
		glVertex3f(p1.x, p1.y, p1.z);//1

		glColor4f(1.0f, 1.0f, 0.0f, 0.5f);
		glVertex3f(p5.x, p5.y, p5.z);
		glVertex3f(p1.x, p1.y, p1.z);//1
		glVertex3f(p4.x, p4.y, p4.z);

		glEnd();

		glBegin(GL_QUADS);
		glColor4f(0.5f, 0.5f, 0.5f, 0.9f);
		glVertex3f(p4.x, p4.y, p4.z);
		glVertex3f(p3.x, p3.y, p3.z);
		glVertex3f(p2.x, p2.y, p2.z);
		glVertex3f(p1.x, p1.y, p1.z);//1
		glEnd();
		glDisable(GL_MULTISAMPLE);

	}
};


class RandyTxt {
public:
	RandyTxt() {};
	~RandyTxt() {};

	char* randyCharSrc = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_&%#@*";
	float textDisplayDecay = 0.0f;
	float shouldDecayTimer = 0.0f;
	float textDisplayTimer = 0.0f;
	float procRevTimer = 0.0f;
	int textDisplayIdx = 0;
	int textDisplayRevIdx = 0;
	float textDisplayTimerNextIntrv = 0.1f;
	float textDisplayTimerNextIncr = 0.1f;


	//const char* selectedCtrlTarg = "";
	std::string selectedCtrlTarg = "";
	std::string selectedCtrlTargStr = "";
	std::string selectedCtrlTargBuilder = "";
	std::string selectedCtrlValueBuilder = "";
	std::string valueKey = "";

	int valLen = 0;
	bool shouldInc = true;
	bool shouldDecay = false;

	char getRandyChar();
	void reset(const char* txt, std::string valK, float interv, float incr);
	// try/catch review: name tables (envNameMap, envSrcNames) have null slots; a null char* turned
	// into std::string is a crash, not an exception. This overload (picked for char*/char* calls)
	// and the null check in reset() make a missing name show as "".
	void reset(const char* txt, const char* valK, float interv, float incr) {
		reset(txt ? txt : "", std::string(valK ? valK : ""), interv, incr);
	}
	void processTextRev();
	void processText();


	

};


//class NVGView : public COpenGLView, public Animation::IAnimationTarget, public REventable, public RDataConsumer, public IControlListener, public Constants {
class NVGView : public COpenGLView, public Animation::IAnimationTarget, public REventable, public RDataConsumer, public IControlListener {
	public:
		void drawMoreInfoWindow();
		//MoreInfoButton* moreInfoBtn;
		void onMoreInfoBtnEnter();
		void onMoreInfoBtnExit();
		void onMoreInfoBtnDown();

		NVGView(const CRect& size, BaseController* dx10) : COpenGLView(size), useThread(false) {
		//NVGView(const CRect & size) : COpenGLView(size), moreInfoBtn(new MoreInfoButton(CRect(CPoint(964.0, 313.0), CPoint(149.0, 63.0)), [this]() { this->onMoreInfoBtnEnter(); }, [this]() { this->onMoreInfoBtnExit(); }, [this]() { this->onMoreInfoBtnDown(); })), useThread(false) {

			baseController = dx10;

			std::list<SharedPointer<CView>> views;
			Map::hash<std::string, SharedPointer<CView>> viewMap;
			std::map<std::string, RSBView> RViewsMap;

			

			srand(time(NULL));
			//paramaters = params;
			
			viewTypes["rect"] = 1;
			viewTypes["knob"] = 2;

			//stepCtrlSelectAnim.controlIdx = NULL;
			selectedStepTarg = NULL;

			fMap.newFunction("setupRKnob", this, &RDataConsumer::setupRKnob);
			fMap.newFunction("setupRKnob2", this, &RDataConsumer::setupRKnob2);
			fMap.newFunction("setupRButton", this, &RDataConsumer::setupRButton);
			fMap.newFunction("setupRSlider", this, &RDataConsumer::setupRSlider);
			fMap.newFunction("setupRSliderVert", this, &RDataConsumer::setupRSliderVert);
			fMap.newFunction("setupRSwitchVert", this, &RDataConsumer::setupRSwitchVert);
			fMap.newFunction("setupSineSymbol", this, &RDataConsumer::setupSineSymbol);
			fMap.newFunction("setupImage", this, &RDataConsumer::setupImage);
			fMap.newFunction("setupRClickButton", this, &RDataConsumer::setupRCButton);
			fMap.newFunction("setupStepCtrlBtn", this, &RDataConsumer::setupStepCtrlBtn);
			//REvents Draw = REvents();
			
			//animationBank[RESET_BUTTON_ANIM] = &NVGView::blinkButton1;
			

			nvgAnimations[BUTTON_BLINK_] = &NVGView::buttonBlink;
			nvgAnimations[BUTTON_BLINK_FASTER] = &NVGView::stepTargBlink;
			nvgAnimations[BUTTON_GLOW_] = &NVGView::buttonGlow;
			nvgAnimations[BUTTON_BLINK_FASTEST] = &NVGView::buttonBlinkFastest;
			


			selectedCtrlTarg = "";
			selectedCtrlValue = "";

			envSrcNames[0] = "";
			envSrcNames[1] = "AMP ENV";
			envSrcNames[2] = "ENV 2";
			envSrcNames[3] = "ENV 3";
			envSrcNames[4] = "LFO 1";
			envSrcNames[5] = "LFO 2";
			envSrcNames[6] = "LFO 3";
			selectedEnvSrc = 0;

			envNameMap[ENV_1_ATTACK] = "AMP_ATTACK";
			envNameMap[ENV_1_DECAY] = "AMP_DECAY";
			envNameMap[ENV_1_RELEASE] = "AMP_RELEASE";
			envNameMap[ENV_1_DECAYTOGGLE] = "AMP_DECAY_TOGGLE";
			envNameMap[ENV_1_DELAY] = "ENV_1_DELAY";
			envNameMap[ENV_2_ATTACK] = "ENV_2_ATTACK";
			envNameMap[ENV_2_DECAY] = "ENV_2_DECAY";
			envNameMap[ENV_2_RELEASE] = "ENV_2_RELEASE";
			envNameMap[ENV_2_DECAYTOGGLE] = "ENV_2_DECAY_TOGGLE";
			envNameMap[ENV_2_DELAY] = "ENV_2_DELAY";
			envNameMap[ENV_3_ATTACK] = "ENV_3_ATTAC";
			envNameMap[ENV_3_DECAY] = "ENV_3_DECAY";
			envNameMap[ENV_3_RELEASE] = "ENV_3_RELEASE";
			envNameMap[ENV_3_DECAYTOGGLE] = "ENV_3_DECAY_TOGGLE";
			envNameMap[ENV_3_DELAY] = "ENV_3_DELAY";
			envNameMap[LFO_1_RATE] = "LFO_1_RATE";
			envNameMap[LFO_1_AMP] = "LFO_1_AMP";
			envNameMap[LFO_1_TYPE] = "LFO_1_TYPE";
			envNameMap[LFO_1_BPM] = "LFO_1_BPM";
			envNameMap[LFO_1_DELAY] = "LFO_1_DELAY";
			envNameMap[LFO_2_RATE] = "LFO_2_RATE";
			envNameMap[LFO_2_AMP] = "LFO_2_AMP";
			envNameMap[LFO_2_TYPE] = "LFO_2_TYPE";
			envNameMap[LFO_2_BPM] = "LFO_2_BPM";
			envNameMap[LFO_2_DELAY] = "LFO_2_DELAY";
			envNameMap[LFO_3_RATE] = "LFO_3_RATE";
			envNameMap[LFO_3_AMP] = "LFO_3_AMP";
			envNameMap[LFO_3_TYPE] = "LFO_3_TYPE";
			envNameMap[LFO_3_BPM] = "LFO_3_BPM";
			envNameMap[LFO_3_DELAY] = "LFO_3_DELAY";
			envNameMap[WIDTH_RESET] = "WIDTH_RESET";
			envNameMap[TUNE_RESET] = "TUNE_RESET";
			envNameMap[FINE_RESET] = "FINE_RESET";
			//envNameMap[MOD_GAIN_RESET] = "MOD_GAIN_RESET";
			envNameMap[MOD_WIDTH_RESET] = "MOD_WIDTH_RESET";
			envNameMap[MOD_TUNE_RESET] = "MOD_TUNE_RESET";
			envNameMap[MOD_FINE_RESET] = "MOD_FINE_RESET";
			//envNameMap[TOGGLE_GEN_VIEW] = "TOGGLE_GEN_VIEW";
			envNameMap[STEP_CTRL_TARG] = "STEP_CTRL_TARG";
			envNameMap[LFO_1_PHASE] = "LFO_1_PHASE";
			envNameMap[LFO_2_PHASE] = "LFO_2_PHASE";
			envNameMap[LFO_3_PHASE] = "LFO_3_PHASE";
			envNameMap[STEP_CTRL_1] = "STEP_CTRL_1";
			envNameMap[STEP_CTRL_2] = "STEP_CTRL_2";
			envNameMap[STEP_CTRL_3] = "STEP_CTRL_3";
			envNameMap[STEP_CTRL_4] = "STEP_CTRL_4";
			envNameMap[STEP_CTRL_5] = "STEP_CTRL_5";
			envNameMap[STEP_CTRL_6] = "STEP_CTRL_6";
			envNameMap[STEP_CTRL_7] = "STEP_CTRL_7";
			envNameMap[STEP_CTRL_8] = "STEP_CTRL_8";
			envNameMap[STEP_CTRL_9] = "STEP_CTRL_9";
			envNameMap[STEP_CTRL_10] = "STEP_CTRL_10";
			envNameMap[STEP_CTRL_11] = "STEP_CTRL_11";
			envNameMap[STEP_CTRL_12] = "STEP_CTRL_12";

			envNameMap[SYNC_GEN_MOD_TUNE] = "SYNC_TUNE";
			envNameMap[ENV_1_DECAYTOGGLE] = "AMP_SYNC_DECAY";
			envNameMap[ENV_2_DECAYTOGGLE] = "ENV_2_SYNC_DECAY";
			envNameMap[ENV_3_DECAYTOGGLE] = "ENV_3_SYNC_DECAY";
			envNameMap[LFO_1_BPM] = "LFO_1_SYNC_BPM";
			envNameMap[LFO_2_BPM] = "LFO_2_SYNC_BPM";
			envNameMap[LFO_3_BPM] = "LFO_3_SYNC_BPM";

			//targDisplayTxt.reset("", "", 0.1f, 0.05f);
			//targValueTxt.reset("", "", 0.1f, 0.05f);

			//GLShader testShader(vsLoc, fsLoc);
			
			for (int i = 0; i < 1000; i++) {
				viewsReady[i] = false;
			}	

			baseController->nvgViewReady = true;

			
		};

		std::string getCopyright();

		std::string version = RSVERSION;

		std::string copyw = getCopyright();

		BaseController* baseController = NULL;
		//DX10Controller* dx10Controller;

		//virtual void willClose(VST3Editor* editor) override;
		//void DX10Controller::willClose(VST3Editor* editor) {
			//nvgViewController->valueChanged
		//	editor->forget();
		//}

		//const char* vsLoc = "D:/vst3sdk/VST3_SDK/public.sdk/MyProjects/SynthProjects/Windows/RSSynth001donnaMedina/fwrksrc/vertShader.vs";
		//const char* fsLoc = "D:/vst3sdk/VST3_SDK/public.sdk/MyProjects/SynthProjects/Windows/RSSynth001donnaMedina/fwrksrc/fragShader.fs";
		//GLShader testShader;

		//BaseController* baseController;
		bool regKeyLegit = true;
		PNGBitmapBuffer cbuf;
		unsigned char charBuffer[205433] = { 0 };
		char* charBuffy;
		bool* isCreatedPtr = nullptr;
		void resetBKG();

		GLPyramid testPyr;
		GLPyramid testPyr2;

		RandyTxt targDisplayTxt;
		RandyTxt targValueTxt;

		//PARAMDEF MUST BE THE SAME AS IN BASECONTROLLER
		std::string NVGParamDef[PARTPARAMS]{
			"OSC_",
			"WIDTH_",
			"WIDTH_ENV",
			"WIDTH_ENV_SRC",
			"TUNE_",
			"TUNE_ENV",
			"TUNE_ENV_SRC",
			"FINE_",
			"FINE_ENV",
			"FINE_ENV_SRC",
			"OCT_",
			"OCT_ENV",
			"OCT_ENV_SRC",
			"INT_",
			"INT_ENV",
			"INT_ENV_SRC",
			"MOD_OSC_",
			"MOD_TYPE_",
			"MOD_GAIN_",
			"MOD_GAIN_ENV",
			"MOD_GAIN_ENV_SRC",
			"MOD_WIDTH_",
			"MOD_WIDTH_ENV",
			"MOD_WIDTH_ENV_SRC",
			"MOD_TUNE_",
			"MOD_TUNE_ENV",
			"MOD_TUNE_ENV_SRC",
			"MOD_FINE_",
			"MOD_FINE_ENV",
			"MOD_FINE_ENV_SRC",
			"MOD_OCT_",
			"MOD_OCT_ENV",
			"MOD_OCT_ENV_SRC",
			"MOD_INT_",
			"MOD_INT_ENV",
			"MOD_INT_ENV_SRC",
			"MOD_BEAT_",
			"MOD_BEAT_ENV",
			"MOD_BEAT_ENV_SRC",
			"GAIN_",
			"GAIN_ENV",
			"GAIN_ENV_SRC",
			"FILTER_",
			"FILTER_ENV",
			"FILTER_ENV_SRC",
			"FILTER_Q",
			"AUX_10",
			"AUX_11",
			"AUX_12",
			"MOD_OSC_GAIN",
			"SYNC_GEN_MOD_TUNE",
			"SYNC_GEN_MOD_FINE",
			"PAN_LR",
			"PAN_LR_ENV",
			"PAN_LR_ENV_SRC"
		};


		char* envNameMap[1000] = { nullptr };

		// try/catch review (B-33): bounds- and null-safe lookups used by the GUI handlers instead of
		// raw indexing (out-of-range / null access is not a C++ exception, so the old try/catch
		// blocks around these never caught anything).
		const char* paramName(int pid) const { return (pid >= 0 && pid < PARTPARAMS) ? NVGParamDef[pid].c_str() : ""; }
		const char* envName(int pid) const { return (pid >= 0 && pid < 1000 && envNameMap[pid]) ? envNameMap[pid] : ""; }
		const char* envSrcName(int i) const { return (i >= 0 && i < 7 && envSrcNames[i]) ? envSrcNames[i] : ""; }

		bool partParamsReady = false;

		char* envSrcNames[7] = { nullptr };
		//char* selectedTargNames[300];
		NVGpaint glowPaint;
		NVGpaint lightPaint;

		void valueChanged(CControl* pControl) override;
		void controlEndEdit(CControl* pControl) override;
		typedef struct { 
			int animName;
			int controlIdx;
			int animState;
		} Anima;
		std::vector<Anima> animationReel;
		Anima stepCtrlSelectAnim = { 2, OCT_, 0 };
		int (NVGView::*nvgAnimations[ANIMCOUNT])(RSBView* rsbView);
		const int BUTTON_BLINK_ = 0;
		const int BUTTON_GLOW_ = 1;
		const int BUTTON_BLINK_FASTER = 2;
		const int BUTTON_BLINK_FASTEST = 3;

		int buttonBlinkFastest(RSBView* rsbV);

		CBMoreInfoButton* moreInfoBtn;
		

		bool moreInfoWindowOpen = false;

		RSBView* selectedStepTarg = nullptr;  // B-30: was uninitialised

		int buttonBlink(RSBView* rsbV);
		int buttonGlow(RSBView* rsbV);
		int stepTargBlink(RSBView* rsbV);

		const int ANIMCOUNT = 1;
		const int RESET_BUTTON_ANIM = 0;

		std::vector<int> activeAnim;

		//std::string

		void drawEnv1();
		float env1Att;
		float env1Dec;
		float env1Rel;
		float env1Toggle;
		float env1OriginX = 55.0;
		float env1OriginY = 370.0;
		float env1AttX;
		float env1AttY;
		float env1DecX;
		float env1DecY;
		float env1RelX;
		float env1RelY;
		float env1RelXSize;

		void drawEnv2();
		float env2Att;
		float env2Dec;
		float env2Rel;
		float env2Toggle;
		float env2OriginX = 204.0;
		float env2OriginY = 370.0;
		float env2AttX;
		float env2AttY;
		float env2DecX;
		float env2DecY;
		float env2RelX;
		float env2RelY;
		float env2RelXSize;

		void drawEnv3();
		float env3Att;
		float env3Dec;
		float env3Rel;
		float env3Toggle;
		float env3OriginX = 352.0;
		float env3OriginY = 370.0;
		float env3AttX;
		float env3AttY;
		float env3DecX;
		float env3DecY;
		float env3RelX;
		float env3RelY;
		float env3RelXSize;

		float envYDif = 45.0;
		float envXDif = 42.0;
		float envXBuffer = 10.0;
		float envFinalX = 119;

		void drawEnv4();
		float lfo1Amp;
		int64 lfo1Type;
		double *lfo1Rate;
		float lfo1Slider;
		float lfo1OriginX = 494.0;
		float lfo1OriginStartX = 494.0;
		float lfo1OriginY = 344.0;
		float lfo1Max = 634.0;
		float lfo1HMax = 0.0;
		float t_lfo1Width = 0.0;
		float lfo1oX = 0.0;
		float lfo1oY = 0.0;
		float lfo1oX2 = 0.0;
		float lfo1oX2Handle = 0.0;
		float lfo1Height = 0.0;

		void drawEnv5();
		float lfo2Amp;
		int64 lfo2Type;
		double *lfo2Rate;
		float lfo2Slider;
		float lfo2OriginX = 642.0;
		float lfo2OriginStartX = 642.0;
		float lfo2OriginY = 344.0;
		float lfo2Max = 782.0;
		float lfo2HMax = 0.0;
		float t_lfo2Width = 0.0;
		float lfo2oX = 0.0;
		float lfo2oY = 0.0;
		float lfo2oX2 = 0.0;
		float lfo2oX2Handle = 0.0;
		float lfo2Height = 0.0;

		void drawEnv6();
		float lfo3Amp;
		int64 lfo3Type;
		double *lfo3Rate;
		float lfo3Slider;

		float lfo3OriginX = 792.0;
		float lfo3OriginStartX = 792.0;
		float lfo3OriginY = 344.0;
		float lfo3Max = 932.0;
		float lfo3HMax = 0.0;
		float t_lfo3Width = 0.0;
		float lfo3oX = 0.0;
		float lfo3oY = 0.0;
		float lfo3oX2 = 0.0;
		float lfo3oX2Handle = 0.0;
		float lfo3Height = 0.0;

		float lfoWidth = 140.0;
		float lfoHeight = 46.0;
		float lfoMinHeight = 5.0;

		
		int(NVGView::*animationBank[ANIMBANKSIZE])(RSOGView* rgv);

		//int blinkButton1(RSOGView* rgv);

		int fontInt;

		//ParameterContainer* paramaters;
		

		REvents fMap;
		REvents Draw;

		RSBView* viewOb;
		CView* cBView;

		RSBView* knobOb;
		CBKnob* cBKnob;

		RSBView* btnOb;
		CBButton* cBBtn;
		CBStepCtrlButton* cBStepCtrl;

		RSBView* clickBtnOb;
		CBButton* clickCBBtn;

		RSBView* sldrOb;
		CBSlider* cBSldr;

		RSBView* vSwxOb;
		CBVerticalSwitch* cBVswx;

		std::vector<CBKnob*> gen2Ctrls;

		bool badReg = false;

		char* randyCharSrc ="ABCDEFGHIJKLMNOPQRSTUVWXYZ_&%#@*";
		float textDisplayDecay = 0.0f;
		float textDisplayTimer = 0.0f;
		int textDisplayIdx = 0;
		float textDisplayTimerNextIntrv = 0.1f;
		char getRandyChar();
		char* selectedCtrlTarg;
		std::string selectedCtrlTargBuilder = "";
		std::string selectedCtrlValueBuilder = "";
		void onBadReg(int pid, double state);
		void onOnOffBtn(int pid, int state);
		void onResetButton(int pid);
		void onEnvSrc(int pid, int envSrc);
		void onEnv(int pid, std::string msg);
		void onEnvBipolar(int pid, std::string msg);
		void onPanEnv(int pid, std::string msg);
		void onEnvLev(int pid, std::string msg);
		void onChangeModType(int pid, int envSrc);
		void onChangeOSCType(int pid, int envSrc);
		void onTuneSwitch(int pid, std::string interv);
		void onChangeLFOOSCType(int pid, int envSrc);
		void onChangeStepTunerType(int pid, int envSrc);
		void onStepCtrlButton(int pid);
		void onStepCtrlSwitch(int pid);
		void onToggleButton(int pid);

		int selectedEnvSrc;
		int selectedCtrlPID;
		std::string selectedCtrlValue;
		float beatModOn = 0.0f;
		float beatModOnPrev = 0.0f;
		float beatModOnLev = 0.0f;


		GLint bufs, samples;

		std::vector<RSBView*> RDrawViews;
		std::vector<std::string> RDrawVector[3];
		std::vector<std::string> RDrawVectorOne;
		//std::vector<int> RDrawVector;

		std::map<std::string, RSBView> RViewsMap;


		Map::hash<std::string, int> viewTypes;

		//virtual void onLinkGen1Up() override;
		virtual void onRCButtonUp(RDataConsumer* rdc) override;

		//virtual void onDrawNvg(const REvent&) override;
		virtual void drawBkg(RData& reIn) override;
		virtual void drawKnob(RData& reIn) override;
		virtual void drawKnob2(RData& reIn) override;
		virtual void drawButton(RData& reIn) override;
		virtual void drawCButton(RData& reIn) override;
		virtual void drawSlider(RData& reIn) override;
		virtual void drawSliderVert(RData& reIn) override;
		virtual void drawSwitchVert(RData& reIn) override;
		virtual void drawStepCtrlBtn(RData& reIn) override;
		virtual void drawImage(RData& reIn) override;
		virtual void drawMoreInfoBtn(RData& reIn) override;

		
		
		virtual void drawSineSymbol(RData& reIn) override;
		void drawBeatModOn();
		void drawBadReg();
		void drawCtrlValueOutput();
		void drawSelctedStepTarg();

		int moreInfoPhase = 0;

		void setupDisplayActive(CView* cvp);
		void setDisplayInactive(CView* cvp);

		void setDisplayActive(CView* cvp, void (RDataConsumer::*handler)(RData& e));
		void toggleDisplayActive(CView* cvp, void (RDataConsumer::*handler)(RData& e) );

		virtual void setupRKnob() override;
		virtual void setupRButton() override;
		void setupRKnob2();
		void setupRCButton();
		void setupRSlider();
		void setupRSliderVert();
		void setupRSwitchVert();
		void setupStepCtrlBtn();
		void setupImage();
		void setupMoreInfoBtn();
		void setupShutWinBtn();
		void setupSineSymbol();


		std::string oscType;

		int pngType;

		void initResetWidthAnim(int pid);

		virtual void drawBlank(RData & obj) override;

		void getViewAttr(CView * view, IdStringPtr attrName, std::string & value);
		
		CControl* controlMap[1000] = { nullptr };
		RSBView* viewIndex[1000] = { nullptr };
		// B-33: viewIndex[pid] is null for any pid without a registered view
		RSBView* viewAt(int pid) const { return (pid >= 0 && pid < 1000) ? viewIndex[pid] : nullptr; }
		

		bool viewsReady[1000];

		int resetSwitch[RESETPARAMCOUNT] = {
		WIDTH_,
		TUNE_,
		FINE_,
		MOD_GAIN_,
		MOD_WIDTH_,
		MOD_TUNE_,
		MOD_FINE_
		};

		REvents Events;

		CFontRef luconFont;

		virtual void animationStart(CView* view, IdStringPtr name)  override;
		
		virtual void animationTick(CView* view, IdStringPtr name, float pos) override;

		virtual void animationFinished(CView* view, IdStringPtr name, bool wasCanceled) override;

		bool attached(CView* parent) override;

		bool removed(CView* parent) override;

		bool shouldDraw = true;

		void platformOpenGLViewCreated() override;

		void platformOpenGLViewWillDestroy() override;

		void platformOpenGLViewSizeChanged() override;

		void drawOpenGL(const CRect& updateRect) override;

		void drawOpenGLThreaded();

		void hideGen2();

		void showGen2();

		PixelFormat* getPixelFormat() override;

		NVGcontext* vg;
		
		CControl* control;

		std::list<SharedPointer<CView> > views;

		Map::hash<std::string, CView*> viewMap;

		Map::hash<int, RSOGView*> setupViewsMap;

		Map::hash<std::string, CBKnob*> knobMap;

		Map::hash<std::string, CBButton*> buttonMap;

		Map::hash<std::string, CBButton*> cButtonMap;

		Map::hash<std::string, CBSlider*> sliderMap;

		Map::hash<std::string, CBVerticalSwitch*> vertSwitchMap;

		Map::hash < std::string, CBStepCtrlButton*> stepCtrlButtonMap;

		//Map::hash<std::string, CView*> viewMap;

		std::map<std::string, int> viewMapKeys;

		std::list<SharedPointer<CView> >::const_iterator viewListIter;

		void killThread();

		void setThreaded(bool state);

		int images[128] = { 0 };
		const int NONE = 0;
		const int SINE_ICON = 1;
		const int SAW_ICON = 2;
		const int TRI_ICON = 3;
		const int SQUARE_ICON = 4;
		const int KNOB_BKG1 = 5;
		const int KNOB2_BKG1 = 6;
		const int ATTACK_OVERLAY = 7;
		const int DECAY_OVERLAY = 8;
		const int RELEASE_OVERLAY = 9;
		const int MAIN_ROW_BKG = 10;
		const int DM_ABOUT_WINDOW = 11;


		void drawSFML(HWND hwnd, const CRect& updateRect) ;//unused, remove from copenglview

		void setupPNG( std::list<SharedPointer<CView> >::const_iterator viewListIter, int intKey );

		double outputD;

		

		CMouseEventResult onMouseDown(CPoint & where, const CButtonState & buttons) SMTG_OVERRIDE;
		CMouseEventResult onMouseUp(CPoint & where, const CButtonState & buttons) SMTG_OVERRIDE;

		std::hash<std::string> hashFunc;
		uint32_t getAttrSize;
		char* getAttrTemp;

		//typedef struct NVGcontext* vgc vg;

		//struct NVGcontext* vg;

		void setOutputD( double dIn) {
			outputD = dIn;
		}

		//wave view
		std::vector<float> oscScopeSmp;
		std::vector<float> oscScopeSX;
		std::vector<float> oscScopeSY;
		std::vector<int>::iterator iter;

		float oscScopeDx;
		float oscScopeH;
		float oscScopeW;
		float oscScopeX;
		float oscScopeY;
		int  oscScopeI;
		float oscStartY = 0.5f;
		float oscMagn1 = 1.5f;
		float oscMagn2 = 0.8f;
		//struct NVGpaint* oscScopeBg;
		//NVGcontext* vg;
		//SFClass SFC;

		void onShutWinBtnDown();

		void setSet(void * parent);

		int runNVGView(int id);

		int XXX;
		
		
		
		//virtual int32_t onKeyDown(VstKeyCode &  keyCode);

	
	protected:
		/*
		class Thread : public FThread
		{
		public:
			Thread(TestOpenGLView* openGLView): FThread("OpenGLDrawThread"), openGLView(openGLView), cancelDrawLoop(false){}
			uint32 entry() override{
				while (cancelDrawLoop == false){
					openGLView->drawOpenGLThreaded();
					FThreadSleep(16);
				}
				return 0;
			}

			void stop(){
				cancelDrawLoop = true;
			}
		};
		void stop();
			
		protected:
			TestOpenGLView* openGLView;
			volatile bool cancelDrawLoop;
		};
		*/

	
		float xRotation;
		float yRotation;
		float zRotation;

		bool useThread;

		void drawOGL();
		//sf::Thread* thread;
		//Thread* thread;
	};



}
}
}