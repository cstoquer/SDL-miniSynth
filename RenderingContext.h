#ifndef _RENDERING_CONTEXT__H
#define _RENDERING_CONTEXT__H

#include <stdint.h>          // integer type aliases
#include "SDL.h"             // SDL lib

//████████████████████████████████████████████████████
//████▀▄▄▄ ████████████████▀████████████████████▀█████
//███ ██████▀▄▄▄▄▀█▄ ▀▄▄▀█▄ ▄▄▄██▀▄▄▄▄▀█▄ ██ ▄█▄ ▄▄▄██
//███ ██████ ████ ██ ███ ██ █████ ▄▄▄▄▄███  ████ █████
//████▄▀▀▀▄█▄▀▀▀▀▄█▀ ▀█▀ ▀█▄▀▀▀▄█▄▀▀▀▀▀█▀ ██ ▀██▄▀▀▀▄█
//████████████████████████████████████████████████████

class RenderingContext {
private:
	SDL_Surface* context;
	SDL_Rect     clip;
	SDL_Rect     translate;
	uint32_t     bgColor;
	int          _transparency[3];

public:
	RenderingContext(SDL_Surface* ctx) {
		context = ctx;
		bgColor = SDL_MapRGB(context->format, 0, 0, 0);
		_transparency[0] = 0;
		_transparency[1] = 0;
		_transparency[2] = 0;
	};

	void clear() {
		SDL_FillRect(context, 0, bgColor);
	};
	void update() {
		SDL_Flip(context);
	};
	void backgroundColor(int r, int g, int b) {
		bgColor = SDL_MapRGB(context->format, r, g, b);
	};
	void transparency(int r, int g, int b) {
		_transparency[0] = r;
		_transparency[1] = g;
		_transparency[2] = b;
	};
	void drawImage(SDL_Surface* img) {
		// TODO: do we need this?
		translate.x = 0;
		translate.y = 0;

		SDL_SetColorKey(img, SDL_SRCCOLORKEY, SDL_MapRGB(img->format, _transparency[0], _transparency[1], _transparency[2]));
		SDL_BlitSurface(img, NULL, context, &translate); // draw bitmap
	};
	void drawImage(SDL_Surface* img, int x, int y) {
		translate.x = x;
		translate.y = y;

		SDL_SetColorKey(img, SDL_SRCCOLORKEY, SDL_MapRGB(img->format, _transparency[0], _transparency[1], _transparency[2]));
		SDL_BlitSurface(img, NULL, context, &translate); // draw bitmap
	};
	void drawImage(SDL_Surface* img, int sx, int sy, int sw, int sh, int tx, int ty) {
		clip.x = sx;
		clip.y = sy;
		clip.w = sw;
		clip.h = sh;

		translate.x = tx;
		translate.y = ty;

		SDL_SetColorKey(img, SDL_SRCCOLORKEY, SDL_MapRGB(img->format, _transparency[0], _transparency[1], _transparency[2]));
		SDL_BlitSurface(img, &clip, context, &translate);
	};

	SDL_Surface* getContext() {
		return context;
	};
};

#endif
