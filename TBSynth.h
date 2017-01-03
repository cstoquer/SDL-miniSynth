#ifndef _TB_SYNTH__H
#define _TB_SYNTH__H

#define AUDIO_RATE 44100.0
#define CONTROL_RATE 689.0625

#include "DecayEnvelope.h"

class TBSynth {
private:
	// counter for Kontrol Rate calculation
	int krCounter;

	// clock generator
	float clock_pos;
	float clock_inc;

	// sequencer
	int   seq_pos;
	float seq_pitch[8];
	bool  seq_trigger[8];
	// bool  seq_accent[8];
	// bool  seq_glide[8];

	// pulse wave oscillator
	float osc_pos;
	float osc_width;
	float osc_freq;
	float osc_out;

	// filter
	float filter_state;
	float filter_cut;
	float filter_res;
	float filter_amt;
	float filter_out;



public:
	DecayEnvelope envAmp;
	DecayEnvelope envFilter;

	TBSynth() {
		krCounter    = 0;
		clock_pos    = 0.0;
		clock_inc    = 0.0;
		seq_pos      = 0;

		seqInitPitch(440,  440,  440,  330,   220,  780,  330,  500);
		seqinitTrig(true, true, true, true, true, true, true, true);

		osc_pos      = 0.0;
		osc_width    = 0.5;
		osc_freq     = 440.0;
		osc_out      = 0.0;
		filter_state = 0.0;
		filter_cut   = 0.0;
		filter_res   = 0.0;
		filter_amt   = 0.0;
		filter_out   = 0.0;

		setTempo(64);
		setCutoff(64);
		setResonance(64);
	};

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	float audioProcess() {
		if (64 == krCounter++) {
			krCounter = 0;
		// ---------- KR generators -----------
		// clock
		clock_pos += clock_inc;
		if (clock_pos >= 1) {
			clock_pos -= 1;
			// clock emit -> increase sequencer
			if (++seq_pos > 7) seq_pos = 0;
			osc_freq = seq_pitch[seq_pos];
			if (seq_trigger[seq_pos]) {
				// seq emit -> trigger enveloppes
				envAmp.trigger();
				envFilter.trigger();
			}
		}
		// amp enveloppe
		envAmp.processKontrol();

		// filter enveloppe
		envFilter.processKontrol();

		// glide

		// TODO

		}
		// ---------- AR generators -----------
		// osc
		osc_pos += osc_freq / AUDIO_RATE;
		if (osc_pos > 1) osc_pos -= 1;
		// osc_out = (osc_pos > osc_width) ? 1 : -1;
		// amp_out = envAmp.out * osc_out;
		osc_out = (osc_pos > osc_width) ? envAmp.out : -envAmp.out;

		// filter

		float fCompCut = filter_cut + envFilter.out * filter_amt;
		if (fCompCut > 1) fCompCut = 1.0;
		float filter_temp = 1 - filter_res * fCompCut;
		filter_state = filter_temp * filter_state - fCompCut * filter_out + fCompCut * osc_out;
		filter_out   = filter_temp * filter_out   + fCompCut * filter_state;

		// efx

		// TODO

		return filter_out;
	};

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	void setTempo(int tempo) {
		clock_inc = (float)(tempo + 70) / (15 * CONTROL_RATE);
	};

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	void setCutoff(int cutoff) {
		filter_cut = (float)cutoff / 127;
	};

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	void setResonance(int resonance) {
		filter_res = 1 - (float)resonance / 127;
	};

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	void setFilterAmount(int amount) {
		filter_amt = (float)amount / 127;
	};

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	void setOscWidth(int value) {
		osc_width = (float)(value + 1) / 256;
	};

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	void seqInitPitch(float a, float b, float c, float d, float e, float f, float g, float h) {
		seq_pitch[0] = a;
		seq_pitch[1] = b;
		seq_pitch[2] = c;
		seq_pitch[3] = d;
		seq_pitch[4] = e;
		seq_pitch[5] = f;
		seq_pitch[6] = g;
		seq_pitch[7] = h;
	};

	void seqPitch(int index, float value) {
		seq_pitch[index] = value;
	};

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	void seqinitTrig(bool a, bool b, bool c, bool d, bool e, bool f, bool g, bool h) {
		seq_trigger[0] = a;
		seq_trigger[1] = b;
		seq_trigger[2] = c;
		seq_trigger[3] = d;
		seq_trigger[4] = e;
		seq_trigger[5] = f;
		seq_trigger[6] = g;
		seq_trigger[7] = h;
	};

	void seqTrigToggle(int index) {
		seq_trigger[index] = !seq_trigger[index];
	};
};

#endif
