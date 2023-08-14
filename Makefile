CC=gcc
CFLAGS=-Wall -Wextra -pedantic

evdev: evdev.o

crear:
	rf -f evdev *.o

