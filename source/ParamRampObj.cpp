#include "ParamRampObj.h"

//changed all (*level)vpointerto level ref
namespace Steinberg {
	namespace Vst {
		namespace rs {
			ParamRampObj::ParamRampObj(){};
			ParamRampObj::ParamRampObj( float &targetValue){
				(level) = targetValue;
			}

			void ParamRampObj::setLevels(float targetValue, float incIn, float* sampleRate) {
				//lastTarg = targ;
				//targ = targetValue;
				targ = targetValue - (level);
				inc = ((1000.0 * targ) / incIn / (*sampleRate));
				if (inc == 0.0f) {
					inc = thresh;
				}
			}

			void ParamRampObj::initLevel(float targetValue) {
				level = targetValue;
			}


			void ParamRampObj::setLevels2(float targetValue, float incIn, float* sampleRate) {
				targ = targetValue;
				inc = 1000 / incIn / (*sampleRate);
				//inc = ((1000.0 * targ) / incIn / (*sampleRate));
				if (inc < thresh) {
					inc = thresh;
				}
			}




			void ParamRampObj::processLevel() {
				if (std::abs(targ - (level)) > thresh) {
					(level) = (level) + (inc * ((targ - (level)) / std::abs(targ - (level))));
				}
				else {
					(level) = targ;
				}
			}

		}
	}
}