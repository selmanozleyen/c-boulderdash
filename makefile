CC = gcc
CFLAGS = -g -Wall

# Detect operating system using uname
UNAME_S := $(shell uname -s)

# Base Allegro libraries
ALLEGRO_LIBS = `pkg-config --cflags --libs allegro-5 allegro_ttf-5 allegro_image-5 allegro_audio-5 allegro_acodec-5`

# Platform-specific libraries
ifeq ($(UNAME_S),Darwin)
    # macOS (Darwin) needs allegro_main
    LIBS = $(ALLEGRO_LIBS) -lallegro_main
else
    # Linux and other platforms don't need allegro_main
    LIBS = $(ALLEGRO_LIBS)
endif

target : all

all:  boulderdash.c
	$(CC) boulderdash.c $(CFLAGS) $(LIBS) -o boulderdash
clean :
	rm -f boulderdash
