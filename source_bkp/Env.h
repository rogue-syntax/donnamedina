

class Env {

public:
	Env();
	~Env();
	float rateValue = 0.0;
	float e = 0.0;
	float env = 1.0;
	float cenv = 1.0;
	float cdec = 0.0;
	float envInc = 0.0;
	float lfoDelay = 0.03f;
	float catt = 0.5;
	float cdecay = 0.5;
	float crel = 0.5;
	float decayReleaseToggle = 0.0;

	float BPM = 0.0;
	int syncBpm = 0;
	float rate = 0.0;
	float level = 0.0;

	int lfoType = 0;

	int toggleGlobal = 0;

	float phaseStart = 1.0;
};