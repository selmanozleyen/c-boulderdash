FLAGS = -g -Wall `pkg-config --cflags --libs allegro-5  allegro_ttf-5 allegro_image-5  allegro_audio-5 allegro_acodec-5 `

target : all

all:  boulderdash.c
	gcc boulderdash.c $(FLAGS) -o boulderdash
clean :
	rm all

