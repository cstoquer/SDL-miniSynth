# What

A small synthesizer test project from my ARM devices (Raspberry PI, Pocket CHIP).

You need to have a MIDI controller plugged in for the application to work properly, 
ideally a `KORG nanoKontrol 2` as it is the device I use for my tests.

# Install and Compile

install SDL library
```bash
sudo apt install libsdl1.2-dev
```

compile and link the liraries
```bash
g++ `sdl-config --cflags` `sdl-config --libs` -o sdlsynth main.cpp
```

# Random notes

```bash
# alsa lib
gcc seqdemo.c -o seqdemo -lasound

# create a shortcut to desktop:
ln -s sdltest /home/pi/Desktop/


# to switch audio output to analog:
amixer cset numid=3 1 # 0=auto, 1=analog, 2=HDMI
```
