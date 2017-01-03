/****************************************************************
SDL Synth
Cedric Stoquer 2016-2017
*****************************************************************/


#include <stdio.h>
#include <stdint.h>          // integer type aliases
#include <math.h>            // pow
#include "SDL.h"             // SDL lib
#include "SDL/SDL_thread.h"  // SDL thread

#include <linux/soundcard.h> // MIDI
#include <unistd.h>          //
#include <fcntl.h>           //

#include "RenderingContext.h"
#include "AmsFont.h"
#include "TBSynth.h"



//█████████████████████████████████
//██▄░░█░░▄██▄▄░▄▄██▄░▄▄▀████▄▄░▄▄█
//███░▄▀▄░█████░█████░███░█████░███
//███░█▄█░█████░█████░███░█████░███
//██▀░▀█▀░▀██▀▀░▀▀██▀░▀▀▄████▀▀░▀▀█
//█████████████████████████████████

int           midiInFileDescriptor;
unsigned char midiBuffer[400]; // can store 100 midi messages
int           midiBufferCount = 0;
SDL_sem*      midiBufferLock = NULL;

//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
void midiBufferPush(unsigned char* inbytes) {
	if (midiBufferCount >= 396) return;
	SDL_SemWait(midiBufferLock);
	midiBuffer[0 + midiBufferCount] = inbytes[0];
	midiBuffer[1 + midiBufferCount] = inbytes[1];
	midiBuffer[2 + midiBufferCount] = inbytes[2];
	midiBuffer[3 + midiBufferCount] = inbytes[3];
	midiBufferCount += 4;
	SDL_SemPost(midiBufferLock);
}

//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
bool midiBufferPop(unsigned char* message) {
	bool result = false;
	SDL_SemWait(midiBufferLock);
	if (midiBufferCount > 0) {
		result = true;
		midiBufferCount -= 4;
		message[0] = midiBuffer[0 + midiBufferCount];
		message[1] = midiBuffer[1 + midiBufferCount];
		message[2] = midiBuffer[2 + midiBufferCount];
		message[3] = midiBuffer[3 + midiBufferCount];
	}
	SDL_SemPost(midiBufferLock);
	return result;
}

//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// NANO KONTROL : factory preset control change numbers

const char nk_fad[8] = { 0,  1,  2,  3,  4,  5,  6,  7}; // faders
const char nk_rot[8] = {16, 17, 18, 19, 20, 21, 22, 23}; // rotary potentiometers

const char nk_Sbt[8] = {32, 33, 34, 35, 36, 37, 38, 39}; // "S" buttons / LEDs
const char nk_Mbt[8] = {48, 49, 50, 51, 52, 53, 54, 55}; // "M" buttons / LEDs
const char nk_Rbt[8] = {64, 65, 66, 67, 68, 69, 70, 71}; // "R" buttons / LEDs
const char nk_trp[5] = {11, 12, 13, 14, 15};             // transport  buttons / LEDs
const char nk_nav[5] = {58, 59, 60, 61, 62};             // navigation buttons / LEDs
const char nk_cycle  = 46;                               // cycle button / LED

//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
int openMidiIn(char const * deviceName) {
	int deviceId = open(deviceName, O_RDONLY);
	if (deviceId == -1) {
		printf("Error: cannot open %s\n", deviceName);
	}
	midiInFileDescriptor = deviceId;
	return deviceId;
}

//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
int openMidiOut(char const * deviceName) {
	// open the OSS device for writing
	int deviceId = open(deviceName, O_WRONLY, 0);
	if (deviceId < 0) printf("Error: cannot open %s\n", deviceName);
	return deviceId;
}

//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
void closeMidiDevice(int deviceId) {
	if (deviceId < 0) return;
	// close the OSS device (optional)
	close(deviceId);
}

//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
void sendMidiMessage(int deviceId, char type, char value1, char value2) {
	// midi message:
	unsigned char msg[3];
	msg[0] = type;
	msg[1] = value1;
	msg[2] = value2;

	// write the MIDI information to the OSS device
	write(deviceId, msg, sizeof(msg));
}

//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
void nanoKontrolPlot(int midiDevice, int x, int y, bool status) {
	char value = status ? 127 : 0;
	if      (y == 0) sendMidiMessage(midiDevice, 0xB0, nk_Sbt[x], value);
	else if (y == 1) sendMidiMessage(midiDevice, 0xB0, nk_Mbt[x], value);
	else if (y == 2) sendMidiMessage(midiDevice, 0xB0, nk_Rbt[x], value);
	else if (y == 3) sendMidiMessage(midiDevice, 0xB0, nk_nav[x], value);
	else if (y == 4) sendMidiMessage(midiDevice, 0xB0, nk_trp[x], value);
	else             sendMidiMessage(midiDevice, 0xB0, nk_cycle,  value);
}

//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
static int midiInThreadFunction(void *data) {
	unsigned char inbytes[4]; // bytes from sequencer driver

	while (1) {
		if (read(midiInFileDescriptor, &inbytes, sizeof(inbytes)) < 0) {
			printf("Error reading Midi device\n");
			exit(1);
		}
 
		midiBufferPush(inbytes);
	}

	return 0;
}


//█████████████████████████████████████████████████
//██▄░░█░░▄████████████████████▄███████████████████
//███░▄▀▄░██▄░██▄░██▀▄▄▄▄░███▄▄░████▀▄▄▄▀░█████████
//███░█▄█░███░███░███▄▄▄▄▀█████░████░██████████████
//██▀░▀█▀░▀██▄▀▀▄░▀█░▀▀▀▀▄███▀▀░▀▀██▄▀▀▀▀▄█████████
//█████████████████████████████████████████████████
float noteToFreq(float noteNumber) {
	return 440 * pow(2, (noteNumber - 69) / 12);
}

//█████████████████████████████████████████████████
//████▄░████████████████▄░█████▄███████████████████
//████░█░███▄░██▄░██▀▄▄▄▀░███▄▄░████▀▄▄▄▄▀█████████
//████░▀░████░███░██░████░█████░████░████░█████████
//██▀░▀█▀░▀██▄▀▀▄░▀█▄▀▀▀▄░▀██▀▀░▀▀██▄▀▀▀▀▄█████████
//█████████████████████████████████████████████████
#define round(x) ((x)>=0?(int16_t)((x)+0.5):(int16_t)((x)-0.5))

float MAIN_VOLUME = 0.5;

inline float cubicLimiterDisto(float x) {
	const float b = -0.25;
	const float c =  1.50;

	if      (x <= -3) return -2;
	else if (x <  -1) return -b * x * x + c * x - b;
	else if (x <=  1) return  x;
	else if (x <   3) return  b * x * x + c * x + b;
	else              return  2;
}
//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// dirty synth implementation
const float SAMPLE_RATE = 44100.0;
/*float SYNTH_pos   = 0.0;
float SYNTH_freq  = 440.0;
float SYNTH_width = 0.5;
bool  SYNTH_mute  = true;

float synthAudioProcess() {
	if (SYNTH_mute) return 0.0;
	SYNTH_pos += SYNTH_freq / SAMPLE_RATE;
	if (SYNTH_pos > 1) SYNTH_pos -= 1;
	return (SYNTH_pos > SYNTH_width) ? 1.0 : -1.0;
}

float synthEvent(int x, int y) {
	SYNTH_freq  = noteToFreq(x + 30);
	SYNTH_width = (float)y / 50;
}*/

TBSynth synth;

//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄

void audioCallback(void* udata, uint8_t* buffer, int len) {
	int16_t* stream = (int16_t*) buffer;
	for (len >>= 1; len; len--) {
		// float out = synthAudioProcess();
		float out = synth.audioProcess();

		// limit output
		out = cubicLimiterDisto(out);
		// apply main amplification
		out = MAIN_VOLUME * out;
		// write in buffer
		*stream++ = round(0xFFF * out);;
	}
}


//█████████████████████████████████████████
//██▄  █  ▄████████████▄███████████████████
//███ ▄▀▄ ██▀▄▄▄▄▀███▄▄ ████▄ ▀▄▄▀█████████
//███ █▄█ ██▀▄▄▄▄ █████ █████ ███ █████████
//██▀ ▀█▀ ▀█▄▀▀▀▄ ▀██▀▀ ▀▀██▀ ▀█▀ ▀████████
//█████████████████████████████████████████

const char * SEQ_NOTE_1 = "STEP 01 NOTE";
const char * SEQ_NOTE_2 = "STEP 02 NOTE";
const char * SEQ_NOTE_3 = "STEP 03 NOTE";
const char * SEQ_NOTE_4 = "STEP 04 NOTE";
const char * SEQ_NOTE_5 = "STEP 05 NOTE";
const char * SEQ_NOTE_6 = "STEP 06 NOTE";
const char * SEQ_NOTE_7 = "STEP 07 NOTE";
const char * SEQ_NOTE_8 = "STEP 08 NOTE";
const char * SEQ_TGGL_1 = "STEP 01 TRIGGER";
const char * SEQ_TGGL_2 = "STEP 02 TRIGGER";
const char * SEQ_TGGL_3 = "STEP 03 TRIGGER";
const char * SEQ_TGGL_4 = "STEP 04 TRIGGER";
const char * SEQ_TGGL_5 = "STEP 05 TRIGGER";
const char * SEQ_TGGL_6 = "STEP 06 TRIGGER";
const char * SEQ_TGGL_7 = "STEP 07 TRIGGER";
const char * SEQ_TGGL_8 = "STEP 08 TRIGGER";
const char * ENV_AMP_RL = "AMP ENVELOPPE RELEASE";
const char * ENV_AMP_CV = "AMP ENVELOPPE CURVATURE";
const char * ENV_FLT_RL = "FILTER ENVELOPPE RELEASE";
const char * ENV_FLT_CV = "FILTER ENVELOPPE CURVATURE";
const char * FILTER_CUT = "FILTER CUTOFF";
const char * FILTER_RES = "FILTER RESONANCE";
const char * FILTER_AMT = "FILTER MODULATION";
const char * OSCL_WIDTH = "OSCILLATOR WIDTH";


int main(int argc, char* argv[]){

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	// SDL INIT

	SDL_Surface* screen = NULL;
	// initialize SDL video and audio
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return 1;

	// make sure SDL cleans up before exit
	atexit(SDL_Quit);

	const SDL_VideoInfo* videoInfo = SDL_GetVideoInfo();

	int systemWidth  = videoInfo->current_w; // should be 320
	int systemHeight = videoInfo->current_h; // should be 240
	Uint8 bpp = videoInfo->vfmt->BitsPerPixel;

	screen = SDL_SetVideoMode(systemWidth, systemHeight, bpp, SDL_SWSURFACE | SDL_FULLSCREEN);
	if (!screen) return 1;

	// init SDL audio
	{
		SDL_AudioSpec audioSpec;
		audioSpec.freq     = SAMPLE_RATE;   // 44.1 kHz
		audioSpec.format   = AUDIO_S16;     // signed 16 bit
		audioSpec.channels = 1;             // mono
		audioSpec.samples  = 512;           // buffer size: 512
		audioSpec.callback = audioCallback;
		SDL_OpenAudio(&audioSpec, NULL);
	}

	SDL_PauseAudio(0); // start audio

	// SDL_ShowCursor(SDL_DISABLE); // hide cursor


	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	// RENDERING

	RenderingContext ctx(screen);
	ctx.backgroundColor(0, 0, 0);
	AmsFont font("/home/pi/Sources/SDLsynth/nesFont.bmp");

	font.paper(0);
	font.pen(26);
	font.locate(1, 1);
	font.print("========== SDL SYNTH v.0.1 ===========");
	font.pen(6);

	ctx.clear();
	ctx.drawImage(font.getImage(), 0, 0);
	ctx.update();

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	// MIDI

	
	int midiOut = openMidiOut("/dev/midi1");
	int midiIn  = openMidiIn("/dev/dmmidi1");
	if (midiInFileDescriptor == -1) printf("Error: cannot open midi device\n");

	// start the thread that interpreting incoming MIDI bytes:
	midiBufferLock = SDL_CreateSemaphore(1);
	SDL_Thread *midiInThread = SDL_CreateThread(midiInThreadFunction, (void *)NULL);
	if (midiInThread == NULL) printf("Error: unable to create MIDI input thread.\n");

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	// MAIN LOOP

	unsigned char midiMsg[4];
	bool displayMsg;
	int run = 400;
	int nkx = 0;

	while (run > 0) {
		// --run;
		SDL_Delay(40); // ~25 FPS
		SDL_Event event;
		int x;
		int y;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT: run = 0; break;
			case SDL_MOUSEMOTION:
				// x = event.motion.x / 8;
				// y = event.motion.y / 8;
				// synthEvent(x, y);
				break;
			case SDL_MOUSEBUTTONUP:
				// SYNTH_mute = true;
				break;
			case SDL_MOUSEBUTTONDOWN:
				x = event.button.x / 8;
				y = event.button.y / 8;
				if (x < 5 && y < 3) run = 0;
				font.locate(x, y);
				font.print("*");
				ctx.clear();
				ctx.drawImage(font.getImage(), 0, 0);
				ctx.update();
				// SYNTH_mute  = false;
				// synthEvent(x, y);
				break;
			}
		}

		//----------------------------------
		// midi message
		displayMsg = false;
		char const * midiMsgCaption;
		while (midiBufferPop(midiMsg)) {
			// printf("received MIDI byte: %d : %d : %d : %d \n", midiMsg[0], midiMsg[1], midiMsg[2], midiMsg[3]);
			switch (midiMsg[1]) {
				// faders
				case 0: synth.seqPitch(0, noteToFreq(midiMsg[2]));   midiMsgCaption = SEQ_NOTE_1; break;
				case 1: synth.seqPitch(1, noteToFreq(midiMsg[2]));   midiMsgCaption = SEQ_NOTE_2; break;
				case 2: synth.seqPitch(2, noteToFreq(midiMsg[2]));   midiMsgCaption = SEQ_NOTE_3; break;
				case 3: synth.seqPitch(3, noteToFreq(midiMsg[2]));   midiMsgCaption = SEQ_NOTE_4; break;
				case 4: synth.seqPitch(4, noteToFreq(midiMsg[2]));   midiMsgCaption = SEQ_NOTE_5; break;
				case 5: synth.seqPitch(5, noteToFreq(midiMsg[2]));   midiMsgCaption = SEQ_NOTE_6; break;
				case 6: synth.seqPitch(6, noteToFreq(midiMsg[2]));   midiMsgCaption = SEQ_NOTE_7; break;
				case 7: synth.seqPitch(7, noteToFreq(midiMsg[2]));   midiMsgCaption = SEQ_NOTE_8; break;

				// potar
				case 16: synth.envAmp.setReleaseTime(midiMsg[2]);    midiMsgCaption = ENV_AMP_RL; break;
				case 17: synth.envAmp.setCurvature(midiMsg[2]);      midiMsgCaption = ENV_AMP_CV; break;
				case 18: synth.envFilter.setReleaseTime(midiMsg[2]); midiMsgCaption = ENV_FLT_RL; break;
				case 19: synth.envFilter.setCurvature(midiMsg[2]);   midiMsgCaption = ENV_FLT_CV; break;
				case 20: synth.setCutoff(midiMsg[2]);                midiMsgCaption = FILTER_CUT; break;
				case 21: synth.setResonance(midiMsg[2]);             midiMsgCaption = FILTER_RES; break;
				case 22: synth.setFilterAmount(midiMsg[2]);          midiMsgCaption = FILTER_AMT; break;
				case 23: synth.setOscWidth(midiMsg[2]);              midiMsgCaption = OSCL_WIDTH; break;

				// button row 3
				case 64: if (midiMsg[2]) synth.seqTrigToggle(0);     midiMsgCaption = SEQ_TGGL_1; break;
				case 65: if (midiMsg[2]) synth.seqTrigToggle(1);     midiMsgCaption = SEQ_TGGL_2; break;
				case 66: if (midiMsg[2]) synth.seqTrigToggle(2);     midiMsgCaption = SEQ_TGGL_3; break;
				case 67: if (midiMsg[2]) synth.seqTrigToggle(3);     midiMsgCaption = SEQ_TGGL_4; break;
				case 68: if (midiMsg[2]) synth.seqTrigToggle(4);     midiMsgCaption = SEQ_TGGL_5; break;
				case 69: if (midiMsg[2]) synth.seqTrigToggle(5);     midiMsgCaption = SEQ_TGGL_6; break;
				case 70: if (midiMsg[2]) synth.seqTrigToggle(6);     midiMsgCaption = SEQ_TGGL_7; break;
				case 71: if (midiMsg[2]) synth.seqTrigToggle(7);     midiMsgCaption = SEQ_TGGL_8; break;
			}
			displayMsg = true;
		}
		if (displayMsg) {
			// only display last message
			font.locate(2, 3);
			font.print(midiMsgCaption);
			// font.printNumber(midiMsg[0]);
			font.print(" : ");
			// font.printNumber(midiMsg[1]);
			// font.print(" : ");
			font.printNumber(midiMsg[2]);
			// font.print(" : ");
			// font.printNumber(midiMsg[3]);
			font.print("       ");
			ctx.clear();
			ctx.drawImage(font.getImage(), 0, 0);
			ctx.update();
		}

		//----------------------------------
		// nanoKontrol light show
		nkx--;
		if (nkx < 0) nkx = 15;
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 3; j++) {
				bool value = ((nkx + i + j) % 16) < 2;
				nanoKontrolPlot(midiOut, i, j, value);
			}
		}
	}

	//▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
	// QUIT

	SDL_KillThread(midiInThread);
	SDL_DestroySemaphore(midiBufferLock);
	closeMidiDevice(midiOut);
	closeMidiDevice(midiIn);
	SDL_Quit();
	return 0;
}
