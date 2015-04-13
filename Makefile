OPTFLAGS=-O2 -ffast-math -ftree-vectorize -ftree-vectorizer-verbose=1

SDLCFLAGS=$(shell sdl2-config --cflags)
SDLLDFLAGS=$(shell sdl2-config --libs)

CFLAGS=-g -Wall -std=gnu99 $(OPTFLAGS) $(SDLCFLAGS)
LDFLAGS=-lm $(SDLLDFLAGS)

fm: fm.o

clean:
	rm -f fm *.o
