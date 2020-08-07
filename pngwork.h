#ifndef PNGWORK_H
#define PNGWORK_H


extern int getprocessed_png(char *filename, unsigned *png_width, unsigned *png_height, char **processed_data);
extern int process_png(unsigned const char *const filedata, int filesize, unsigned *png_width, unsigned *png_height, char **processed_data);

#endif
