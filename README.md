Piano program with FFT waterfall plot, wave graph, piano roll

### To compile on debian based linux:

- Install needed things `sudo apt-get install build-essential libfftw3-dev libsdl2-dev zlib1g-dev`
- Compile run `make -k` in the project directory

### To compile on mac:
- Install brew `/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)`
- Install sdl `brew install sdl2`
- Install fftw3 `brew install fftw`
- You probably already have a compiler installed, if not, a dialog box will pop up for installing clang and some stuff
- To compile run `make -k` in the project directory



### To compile on windows:
Read the instructions in the Makefile and then
run the command `make -k` in the project directory.
If the `make -k` command does not compile the program, try `mingw32-make -k` 


#### An image of the program running on windows:
![Test Image 1](my_program.png)




On mac and some linux distributions the program is little buggy graphically for some reason
