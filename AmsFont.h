#ifndef _AMSTRAD_FONT__H
#define _AMSTRAD_FONT__H

#include "RenderingContext.h"


//█████████████████████████████████████████████████████████
//████▄ ████████████████████▄ ▄▄▄ ███████████████████▀█████
//████ █ ██▄ ▀▄▀▀▄▀█▀▄▄▄▄ ███ ▀ ████▀▄▄▄▄▀██▄ ▀▄▄▀██▄ ▄▄▄██
//████ ▀ ███ ██ ██ ██▄▄▄▄▀███ █▄████ ████ ███ ███ ███ █████
//██▀ ▀█▀ ▀▀ ▀█ ▀█ █ ▀▀▀▀▄██▀ ▀█████▄▀▀▀▀▄██▀ ▀█▀ ▀██▄▀▀▀▄█
//█████████████████████████████████████████████████████████

const int SCREEN_W = 480;
const int SCREEN_H = 272;

const int CHAR_SIZE  = 8;
const int MAX_COLUMN = SCREEN_W / CHAR_SIZE;
const int MAX_LINE   = SCREEN_H / CHAR_SIZE;

class AmsFont {
private:
	SDL_Surface*      font;
	int               _x;
	int               _y;
	SDL_Surface*      _clearRect;
	SDL_Rect          _clearPos;
	int               _paper[3];
	RenderingContext* _pen;

public:
	RenderingContext* ctx;

	AmsFont(char const * fileName) {
		font = SDL_LoadBMP(fileName);
		SDL_Surface* surface = SDL_CreateRGBSurface(0, SCREEN_W, SCREEN_H, 32, 0, 0, 0, 0);
		ctx = new RenderingContext(surface);
		ctx->transparency(0, 0, 0);
		_x = 0;
		_y = 0;
		_clearRect = SDL_CreateRGBSurface(SDL_HWSURFACE, CHAR_SIZE, CHAR_SIZE, 32, 0, 0, 0, 0);
		_paper[0] = 0;
		_paper[1] = 0;
		_paper[2] = 0;
		SDL_Surface* penSurface = SDL_CreateRGBSurface(0, font->h, font->w, 32, 0, 0, 0, 0);
		_pen = new RenderingContext(penSurface);
		pen(0);
	};

	~AmsFont() {
		delete ctx;
		SDL_FreeSurface(font);       // free loaded bitmap
		SDL_FreeSurface(_clearRect); // free other surfaces
	};

	SDL_Surface* getImage() {
		SDL_SetColorKey(ctx->getContext(), SDL_SRCCOLORKEY, SDL_MapRGB(ctx->getContext()->format, 0, 0, 0));
		return ctx->getContext();
	};

	void paper(int code) {
		if (code == -1 || code > 27) {
			_paper[0] = 0;
			_paper[1] = 0;
			_paper[2] = 0;
			return;
		}

		// FIXME: set max to 255
		int r = 127 * ((code / 3) % 3);
		int g = 127 * ((code / 9));
		int b = 127 * (code % 3);

		if (r == 0 && g == 0 && b == 0) b = 1; // no transparency
		_paper[0] = r;
		_paper[1] = g;
		_paper[2] = b;
	};

	void pen(int code) {
		int r, g, b;
		if (code == -1 || code > 27) {
			r = 0; g = 0; b= 0;
		} else {
			r = 127 * ((code / 3) % 3);
			g = 127 * ((code / 9));
			b = 127 * (code % 3);
		}

		if (r == 0 && g == 0 && b == 0) b = 1; // no transparency
		_pen->transparency(255, 255, 255);
		_pen->backgroundColor(r, g, b);
		_pen->clear();
		_pen->drawImage(font);
	};

	void locate(int x, int y) {
		_x = x;
		_y = y;
	};

	void print(unsigned char c) {
		if (c == '\n') {
			_x = 0;
			_y++;
			return;
		}

		int sourceX = (c % 16) * CHAR_SIZE;
		int sourceY = (c / 16) * CHAR_SIZE;
		int destX = _x * CHAR_SIZE;
		int destY = _y * CHAR_SIZE;

		// clear character background
		_clearPos.x = destX;
		_clearPos.y = destY;
		// paper color
		SDL_FillRect(_clearRect, NULL, SDL_MapRGB(ctx->getContext()->format, _paper[0], _paper[1], _paper[2]));
		SDL_BlitSurface(_clearRect, NULL, ctx->getContext(), &_clearPos);
		// draw character
		ctx->drawImage(_pen->getContext(), sourceX, sourceY, CHAR_SIZE, CHAR_SIZE, destX, destY);

		if (++_x >= MAX_COLUMN) {
			_x = 0;
			if (++_y > MAX_LINE) {
				_y--;
				scroll(1);
			}
		}
	};

	void print(char const * text) {
		while (*text) {
			int c = *text++;
			print(c);
		}
	};

	void printNumber(int number) {
		if (number == 0) {
			print("0");
			return;
		}
		if (number < 0) {
			print("-");
			number = -number;
		}
		int size = 0;
		char digits[10];
		while (number) {
			digits[size++] = (number % 10) + 48;
			number = number / 10;
		}
		while (--size >= 0) {
			print(digits[size]);
		}
	};

	void scroll(int) {
		// save current
		// TODO
	};
};

#endif
