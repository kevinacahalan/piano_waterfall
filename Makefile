
ifeq ($(OS),Windows_NT)
# Windows using mingw-w64 toolkit gcc compiler http://mingw-w64.org/

# For fftw3 get the library here: http://www.fftw.org/download.html
# After downloading and unzipping run "dlltool -l libfftw3-3.lib -d libfftw3-3.def" in the library folder.
# Next move the library folder to the locations FFTW3_CFLAGS and FFTW3_LIBS use and then rename the folder 
# to the names FFTW3_CFLAGS and FFTW3_LIBS use. Conversly you could change FFTW3_CFLAGS and FFTW3_LIBS to  
# have the name/location you would would prefer to put/call the library folder.
# Next the take fftw3-3.dll from the library folder and put it in the folder this Makefile is in which also
# to be the location piano_waterfall.exe is put after build.

# For SDL2 get the library here: https://www.libsdl.org/download-2.0.php
# Download the Development library for windows mingw called SDL2-devel-2.#.##-mingw.tar.gz and decompress.
# To decompress use "tar -xzf [thing to decompress]", the tar canmand comes with the mingw-w64 tool kit.
# After decompressing you should get a folder called SDL2-2.#.## with a folder inside called
# x86_64-w64-mingw32. That folder called is what will be referrerd to as the "SDL2 library folder".
# Change the name if this library folder to something good like "libsdl2" and place it where you want.
# Next do the same stuff done with the fftw3 library folder, kind of....
#
# Start by taking SDL2.dll from [location of SDL2 library folder]\bin and put it in the same spot as this
# makefile, the location where fftw3-3.dll was put.
# Then set SDL2_CFLAGS to be -I"[location of SDL2 library folder]\include\SDL2"
# Then set SDL2_LIBS to be -L"[location of SDL2 library folder]\lib" -lmingw32 -lSDL2main -lSDL2 -mwindows -Dmain=SDL_main

# if you don't have your SDL2 and fftw3 library folders called libsdl2 & libfftw3 and located at "C:\"
# changes the flags below:
SDL2_CFLAGS := -I"C:\libsdl2\include\SDL2"
SDL2_LIBS := -L"C:\libsdl2\lib" -lmingw32 -lSDL2main -lSDL2 -mwindows -Dmain=SDL_main
FFTW3_CFLAGS := -I"C:\libfftw3"
FFTW3_LIBS := -L"C:\libfftw3" -lfftw3-3
else
# Linux
SDL2_CFLAGS := $(shell sdl2-config --cflags)
SDL2_LIBS := $(shell sdl2-config --libs)
FFTW3_CFLAGS := 
FFTW3_LIBS := -lfftw3
endif


# does not work, for linux can't static link asound but would work if asound was dynamic
SDL2_STATIC := $(shell sdl2-config --static-libs)


NAME := piano_waterfall
.PHONY: all
all: $(NAME)

$(NAME): Makefile main.c pngwork.c pngwork.h
	gcc -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow -fno-common $(SDL2_CFLAGS) $(FFTW3_CFLAGS)  -g3 -Og -o $@ main.c pngwork.c -lz -lm $(SDL2_LIBS) $(FFTW3_LIBS)

mem_$(NAME): Makefile main.c pngwork.c pngwork.h
	gcc -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow -fno-common $(SDL2_CFLAGS) $(FFTW3_CFLAGS) -g3 -Og -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -o $@ main.c pngwork.c -lz -lm $(SDL2_LIBS) $(FFTW3_LIBS)

und_$(NAME): Makefile main.c pngwork.c pngwork.h
	gcc -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow -fno-common $(SDL2_CFLAGS) $(FFTW3_CFLAGS) -g3 -Og -fsanitize=undefined -o $@ main.c pngwork.c -lz -lm $(SDL2_LIBS) $(FFTW3_LIBS)

static_$(NAME): Makefile main.c pngwork.c pngwork.h
	gcc -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow -fno-common $(SDL2_CFLAGS) $(FFTW3_CFLAGS) -g3 -Og -Wl,-Bdynamic -lasound -Wl,-Bstatic -o $@ main.c pngwork.c -lz -lm $(SDL2_LIBS) $(FFTW3_LIBS)
