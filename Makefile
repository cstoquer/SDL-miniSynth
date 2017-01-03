sdlsynth: main.cpp
	g++ `sdl-config --cflags` `sdl-config --libs` -o sdlsynth main.cpp