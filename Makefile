SDL2_CFLAGS := $(shell sdl2-config --cflags)
SDL2_LIBS := $(shell sdl2-config --libs)
NAME := piano_waterfall

.PHONY: all
all: $(NAME)

$(NAME): Makefile main.c pngwork.c pngwork.h
	gcc -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow -fno-common $(SDL2_CFLAGS) -g3 -Og -o $@ main.c pngwork.c -lz -lfftw3 -lm $(SDL2_LIBS)

mem_$(NAME): Makefile main.c pngwork.c pngwork.h
	gcc -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow -fno-common $(SDL2_CFLAGS) -g3 -Og -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -o $@ main.c pngwork.c -lz -lfftw3 -lm $(SDL2_LIBS)

und_$(NAME): Makefile main.c pngwork.c pngwork.h
	gcc -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow -fno-common $(SDL2_CFLAGS) -g3 -Og -fsanitize=undefined -o $@ main.c pngwork.c -lz -lfftw3 -lm $(SDL2_LIBS)
