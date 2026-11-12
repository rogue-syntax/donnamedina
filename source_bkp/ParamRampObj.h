#pragma once
#include <cmath>

namespace Steinberg {
	namespace Vst {
		namespace rs {
			class ParamRampObj {
			public:
				ParamRampObj();
				ParamRampObj(float &targetValue);
				~ParamRampObj() {};

				float lastTarg = 0.0f;
				float difference;
				float targ = 0.0f;
				float inc = 0.0f;
				float level;
				float thresh = 0.002f;
				float zeroToTargTime = 0.0f;
				float lastTargToNewTime = 0.0f;

				void setLevels(float targetValue, float incIn, float* sampleRate);
				void initLevel(float targetValue);
				//incIn : time from zero to targetLevel in milliseconds
				void setLevels2(float targetValue, float incIn, float* sampleRate);

				void processLevel();

			};

		}
	}
}