#ifndef DECAY_ENVELOPE_H
#define DECAY_ENVELOPE_H

float map(float value, float iMin, float iMax, float oMin, float oMax) {
	return oMin + (oMax - oMin) * (value - iMin) / (iMax - iMin);
}

// const float _decEnvSmooth = 0.02;
// const float _decEnvSmoothInv = 1 - _decEnvSmooth;

class DecayEnvelope {

private:
	float* trig;
	int    t;
	float  a;
	float  b;
	float  raw;
	float  release;
	int    releaseTime; 
	float  curvature;
	bool   stopped;

	void update() {
		// FIXME: if parameters changes while envelope playing,
		//        there will be an audio glich.
		//        process should only use previous output value, not t.
		float r = 30 + release * 470;
		releaseTime = (int)r - 1;
		a = (2 - 4 * curvature) / (r * r);
		b = (4 * curvature - 3) / r;
	}

public:
	float out;

	DecayEnvelope() {
		release = 0.5;
		curvature = 0.3;
		stopped = true;
		t = 0;
		out = 0.0;
		raw = 0.0;
		update();
	}

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	void trigger() {
		stopped = false;
		t = 0;
	}

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	void processKontrol() {
		if (stopped) return;
		if (t++ > releaseTime) {
			stopped = true;
			out = 0.0;
		}
		// raw = a * (float)(t * t) + b * (float)t + 1;

		// built in smoothing filter
		// out = raw * _decEnvSmooth + out * _decEnvSmoothInv;

		out = a * (float)(t * t) + b * (float)t + 1;
	}

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	void setCurvature(int c) {
		// bound curvature in range [0.3 .. 0.7] (or envelope becomes instable)
		curvature = map((float)c, 0, 127, 0.3, 0.7);
		update();
	}

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	void setReleaseTime(int r) {
		release = (float)r / 127;
		update();
	}
};

#endif