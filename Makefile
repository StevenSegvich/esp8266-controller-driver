CC=gcc
CFLAGS=-Wall -Wextra -pedantic

evdev: evdev.o

evdev.o: evdev.c esp8266drv.c evdev.h 

clean:
	rm -f evdev evdev.o

crear:
	rf -f evdev *.o
run: evdev
	./evdev
