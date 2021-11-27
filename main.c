#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <errno.h>
#include <string.h>
#include "pngwork.h"
#include <SDL.h>
#include <complex.h>
#include <fftw3.h>


//To appease dumb buggy VScode c/c++ extension
#ifndef M_PI
//#include </usr/include/math.h>
#define M_PI 123123
#define M_LN2 123123
#define M_2_PI 123123
#endif

//#include <SDL/SDL.h>   //gcc -W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-common -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -g3 -Og -o kview_sdl1 main.c pngwork.c -lz -L/usr/lib/x86_64-linux-gnu -lSDL
//#include <SDL2/SDL.h>  //gcc -W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-common -I/usr/include/SDL2 -D_REENTRANT -g3 -Og -o kview_sdl2 main.c pngwork.c -lz -lSDL2

static int quitting;

#define NOTHING 0

#define COLOR(r,g,b) (((r)<<16)|((g)<<8)|(b))
#define BYTES_PER_PIXEL 3 //could have this taken from pngwork.c
#define rgb24_STRIDE (BYTES_PER_PIXEL * png_width)

#define SAMPLE_RATE 48000
#define MAX_SAMPLES 4096
#define NUM_SAMPLES 512
#define NUM_TIMBRE 4
#define NUM_PITCH 128
#define LOGFREQ_KEY_GAP_WIDTH 1
#define LOGFREQ_KEY_WIDTH 10

#define LOGFREQ_WIDTH (NUM_PITCH*LOGFREQ_KEY_WIDTH)
#define FREQ_WIDTH 1024 //normaly at 1024
#define LOGFREQ_HEIGHT 400 //normaly at 400
#define FREQ_HEIGHT LOGFREQ_HEIGHT
#define FREQ_KEYS_HEIGHT 19
#define LOGFREQ_KEYS_HEIGHT 19
unsigned freq_key_image_height /*= FREQ_KEYS_HEIGHT*/;
unsigned logfreq_key_image_height /*= LOGFREQ_KEYS_HEIGHT*/;
#define YELLOW COLOR(255,255,0)

#define LOGFREQ_RED   0x00000001
#define LOGFREQ_GREEN 0x00000002
#define LOGFREQ_BLUE  0x00000004
#define FREQ_RED      0x00000008
#define FREQ_GREEN    0x00000010
#define FREQ_BLUE     0x00000020

static unsigned enabled_colors = LOGFREQ_RED|LOGFREQ_BLUE|FREQ_RED|FREQ_BLUE;

#if 0
#define SAMPLE_MAX 0x7fff
#define SAMPLE_MIN (-0x7fff-1)
typedef short sample_type;
#define SAMPLE_IS_FLOAT 0
#define SAMPLE_IS_UNSIGNED 0
#define AUDIO_KEVIN AUDIO_S16SYS
#endif

#if 0
#define SAMPLE_MAX 0xffff
#define SAMPLE_MIN 0
typedef unsigned short sample_type;
#define SAMPLE_IS_FLOAT 0
#define SAMPLE_IS_UNSIGNED 1
#define AUDIO_KEVIN AUDIO_U16SYS
#endif

#if 0
#define SAMPLE_MAX 0x7fffffff
#define SAMPLE_MIN (-0x7fffffff-1)
typedef int sample_type;
#define SAMPLE_IS_FLOAT 0
#define SAMPLE_IS_UNSIGNED 0
#define AUDIO_KEVIN AUDIO_S32SYS
#endif

#if 0
#define SAMPLE_MAX 0xffffffff
#define SAMPLE_MIN 0
typedef unsigned sample_type;
#define SAMPLE_IS_FLOAT 0
#define SAMPLE_IS_UNSIGNED 1
#define AUDIO_KEVIN AUDIO_U32SYS
#endif

#if 1
#define SAMPLE_MAX 1.0
#define SAMPLE_MIN (-1.0)
typedef float sample_type;
#define SAMPLE_IS_FLOAT 1
#define SAMPLE_IS_UNSIGNED 0
#define AUDIO_KEVIN AUDIO_F32SYS
#endif

#if 0
#define SAMPLE_MAX 0xff
#define SAMPLE_MIN 0
typedef unsigned char sample_type;
#define SAMPLE_IS_FLOAT 0
#define SAMPLE_IS_UNSIGNED 1
#define AUDIO_KEVIN AUDIO_U8
#endif

#if 0
#define SAMPLE_MAX 127
#define SAMPLE_MIN (-128)
typedef unsigned char sample_type;
#define SAMPLE_IS_FLOAT 0
#define SAMPLE_IS_UNSIGNED 0
#define AUDIO_KEVIN AUDIO_U8
#endif

static sample_type waves[NUM_TIMBRE][NUM_PITCH/*128*/][MAX_SAMPLES];
static unsigned num_samples[128];
static unsigned wave_indexs[128];
static unsigned note_to_play = 0;
static unsigned note_type = 0;
static unsigned octave = 4;
struct msg{
	unsigned value;
	unsigned keybits[4];
	unsigned audio_chunks_drawn;
	sample_type audio[];
};

// Lookup array for going from SDL key codes to notes. If a Key does not correspond to a
// note, that key is considered note 0. Note 0 is a nobody note.
//
static char _key_to_midi[512] = {
	[SDL_SCANCODE_TAB] 			= 5,
		[SDL_SCANCODE_1] 		= 6,//black
	[SDL_SCANCODE_Q]			= 7,
		[SDL_SCANCODE_2]		= 8,//black
	[SDL_SCANCODE_W] 			= 9,
		[SDL_SCANCODE_3] 		= 10,//black
	[SDL_SCANCODE_E] 			= 11,
	[SDL_SCANCODE_R] 			= 12,
		[SDL_SCANCODE_5] 		= 13,//black
	[SDL_SCANCODE_T] 			= 14,
		[SDL_SCANCODE_6] 		= 15,//black
	[SDL_SCANCODE_Y] 			= 16,
	[SDL_SCANCODE_U] 			= 17,
		[SDL_SCANCODE_8] 		= 18,//black
	[SDL_SCANCODE_I] 			= 19,
		[SDL_SCANCODE_9] 		= 20,//black
	[SDL_SCANCODE_O] 			= 21,
		[SDL_SCANCODE_0] 		= 22,//black
	[SDL_SCANCODE_P] 			= 23,
	[SDL_SCANCODE_LEFTBRACKET] 	= 24,
		[SDL_SCANCODE_EQUALS] 	= 25,//black
	[SDL_SCANCODE_RIGHTBRACKET] = 26,
		[SDL_SCANCODE_BACKSPACE]= 27,//black
	[SDL_SCANCODE_BACKSLASH] 	= 28,

	// Random keys used for debugging:
	//
	// [SDL_SCANCODE_SPACE] 	= 1,
	// [SDL_SCANCODE_Z] 		= 2,
	// [SDL_SCANCODE_X] 		= 3,
	// [SDL_SCANCODE_C] 		= 4,
	// [SDL_SCANCODE_V] 		= 5,
	// [SDL_SCANCODE_B] 		= 6,
	// [SDL_SCANCODE_N] 		= 7,
	// [SDL_SCANCODE_M] 		= 8,
};

// Turns key number to midi number
static int key_to_midi(int key){
	int m = _key_to_midi[key];
	if (!m)
		return 0;
	m += octave*12;
	if (m > 127)
		return 0;
	
	return m;
}

// There are 128 notes, keysbits is an array of 4 32bit unsigned int's...thats 128bits
static unsigned keybits[4];

// This function is called whenever a key goes up. If that key corresponds to a note, that
// note is turned off.
static void note_key_up(int key){
	int m = key_to_midi(key);
	if (!m)
		return;
	note_to_play = 0; //after multi sound support remove this line

	const unsigned bit_index   = 0x1f & m;
	const unsigned array_index = m >> 5;
	keybits[array_index] &= ~(1u << bit_index);
}

// After some specific keys are checked for in keyboard_down(), this function is called
// for the case the key pushed down is not special. If that key corresponds to a note, that
// note is turned off.
static void note_key_done(int key){
	int m = key_to_midi(key);
	if (!m)
		return;
	note_to_play = m; //after multi sound support remove this line

	const unsigned bit_index   = 0x1f & m;
	const unsigned array_index = m >> 5;

	//check if bit is already set
	if (!(keybits[array_index] & (1u << bit_index))){
		keybits[array_index] |= (1u << bit_index);
	}
}

// Check if specific note/midi is playing
static int is_midi_playing(int m, unsigned *local_keybits){
	const unsigned bit_index   = 0x1f & m;
	const unsigned array_index = m >> 5;

	return local_keybits[array_index] & (1u << bit_index);
}

// Struct for keeping track of a window. The window structs are kept track of in a singly linked list.
typedef struct window_node{
	struct window_node *next;
	void (*destroy)(struct window_node *is); // Pointer to destroy method
	void (*expose)(struct window_node *is); // Pointer to expose method
	unsigned width; // Width of windows in pixels
	unsigned height; // Height of window in pixels
	char *name; // The name that shows up on the window bar
	SDL_Window *win;
	SDL_Renderer *rend;
	SDL_Texture *tex;
	SDL_Texture *tex_extra;
}window_node; // typedef does not work til this line
static window_node *list_head = NULL;


static window_node *newgraph_win_and_tex;
static window_node *freq_win_and_tex;
static window_node *logfreq_win_and_tex;

static fftw_complex *fft_in;
static fftw_complex *fft_out;
static fftw_plan fft_sm_plan;
static fftw_plan fft_lg_plan;

static double midi_to_hz(double midi);
static double hz_to_midi(double Hz);
static void display_stuff(window_node *win_node, unsigned *data, unsigned row_index, unsigned key_height);
static void display_newgraph(window_node *win_node, struct msg *msg);


static window_node *add_window_node(void (*destroy)(struct window_node *is), void (*expose)(struct window_node *is)){
	struct window_node *new_node = calloc(1, sizeof *new_node);
	new_node->next = list_head;
	list_head = new_node;
	new_node->destroy = destroy;
	new_node->expose = expose;
	new_node->tex = NULL;
	new_node->tex_extra = NULL;

	return new_node;
}


static window_node *windowID_to_pointer(unsigned windowID){
	window_node *walk = list_head;
	while (walk){
		if (SDL_GetWindowID(walk->win) == windowID)
			return walk;
		walk = walk->next;
	}
	return NULL;
}


static void closewindow(unsigned windowID){
	window_node *walk = list_head;
	window_node *past = NULL;
	while (walk){
		if (SDL_GetWindowID(walk->win) == windowID){
			SDL_DestroyRenderer(walk->rend);
			SDL_DestroyWindow(walk->win);
			if (past){
				past->next = walk->next;
			}else{
				list_head = walk->next;
			}

			//call destroy fucntion pointer, if NULL there is no destroy function for window
			if (walk->destroy)
				walk->destroy(walk);
			memset(walk, 0xee, sizeof *walk);
			
			free(walk);
			return;
		}
		//function to remove link list item
		past = walk;
		walk = walk->next;
	}
	
}


static int magnitude_to_sRGB(double magnitude){
	int chan = sqrt(magnitude) * 256;
	if (chan > 255){
		chan = 255;
	}else if (chan < 0){
		chan = 0;
	}	

	return chan;
}

static double sample_to_double(sample_type s){
	if(SAMPLE_IS_FLOAT)
		return s;
	return (s - (double)SAMPLE_MIN)*(2/(SAMPLE_MAX - (double)SAMPLE_MIN)) - 1.0;
}

static sample_type double_to_sample(double d){
	if(SAMPLE_IS_FLOAT)
		return d;
	long long s = (d + 1)/2 * (SAMPLE_MAX - (double)SAMPLE_MIN) + SAMPLE_MIN;
	if(s > (long long)SAMPLE_MAX || s < (long long)SAMPLE_MIN)
		#ifdef WIN32
		printf("double_to_sample saw %I64d\n",s);
		#else
		printf("double_to_sample saw %lld\n",s);
		#endif
	return s;
}

static double limiter(double x){
	return atan(x) * M_2_PI;
}

static double dsp_window(double x){
	return cos((x-0.5)*2*M_PI)/2 + 0.5;
	//return 0.5 + sin(x*M_PI)/2;

}

static void compute_spectrum(double *restrict const dst, const sample_type *restrict const src, unsigned n, fftw_plan plan, double *restrict const max_mag){
	unsigned i = 0;
	do{
		sample_type s = src[i];
		fft_in[i] = sample_to_double(s) * dsp_window(i/(double)(n-1));
	}while(++i < n);

	fftw_execute(plan);

	i = 0;
	do{
		double magnitude = cabs(fft_out[i]);
		if (magnitude > *max_mag)
			*max_mag = magnitude;
		dst[i] = magnitude / *max_mag;
	}while(++i < n/2);
}

#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))

#define Q_HEIGHT 2048 // Must exceed window height and be power of 2. (64, 128, 256, 512, 1024, 2048)
static unsigned logfreq_current = 0;
static unsigned freq_current = 0;
static double max_sm_magnitude = 0;
static double max_lg_magnitude = 0;
static sample_type audio_for_fft[8192+256] = {0};
static unsigned freq_data[Q_HEIGHT][FREQ_WIDTH];
static unsigned logfreq_data[Q_HEIGHT][LOGFREQ_WIDTH];
/*This function mostly does the moving graphics, and a few other things*/
static void dealwith_userevent(SDL_UserEvent *event){
	struct msg *msg = event->data1;
	logfreq_current = (logfreq_current+1)%Q_HEIGHT;
	freq_current = (freq_current+1)%Q_HEIGHT;
	memset(&logfreq_data[(logfreq_current -1)%Q_HEIGHT][0], 0, LOGFREQ_WIDTH * 1 * sizeof(unsigned));
	memset(&freq_data   [(freq_current    -1)%Q_HEIGHT][0], 0, FREQ_WIDTH    * 1 * sizeof(unsigned));
	size_t i = 1;// the first note happens when i = 5, having i start at i = 5 is maybe a better idea;
	while (i < 128){
		if (is_midi_playing(i, msg->keybits)){
			// log frequency window
			if(enabled_colors & LOGFREQ_BLUE){
				for (size_t j = 1; j < LOGFREQ_KEY_WIDTH -1; j++){
					logfreq_data[(logfreq_current -1)%Q_HEIGHT][i*LOGFREQ_KEY_WIDTH+j] |= 	/*Blue*/0x0000ff;
				}
			}

			// linear frequency window
			if(enabled_colors & FREQ_BLUE){
				unsigned lower = midi_to_hz(i - 0.4)/(48000.0/8192.0);
				unsigned upper = midi_to_hz(i + 0.4)/(48000.0/8192.0);
				if(upper > FREQ_WIDTH - 1){
					upper = FREQ_WIDTH - 1;
					puts("upper was not smaller than FREQ_WIDTH, it was .........");
				}
				for (size_t j = lower; j < upper; j++){
					freq_data[(freq_current -1)%Q_HEIGHT][j] |= 	/*Blue*/0x0000ff;
				}
			}
		}
		i++;
	}


	unsigned long bytes_given = msg->value; //happens to be same as event->data2
	unsigned long samples_given = bytes_given / sizeof(sample_type);
	if(samples_given != 512){
		printf("fail with bytes_given %lu, samples_given %lu\n",bytes_given, samples_given);
		exit(1);
	}

	memmove(audio_for_fft, audio_for_fft + samples_given, sizeof audio_for_fft - bytes_given);
	memcpy(audio_for_fft + ARRAYSIZE(audio_for_fft) - samples_given, msg->audio, bytes_given);

	double sm_mag[1024/2];
	double lg_mag[8192/2];
	compute_spectrum(sm_mag, audio_for_fft + ARRAYSIZE(audio_for_fft)-256-1024, 1024, fft_sm_plan, &max_sm_magnitude);
	compute_spectrum(lg_mag, audio_for_fft + ARRAYSIZE(audio_for_fft)-256-8192, 8192, fft_lg_plan, &max_lg_magnitude);


	if(enabled_colors & FREQ_GREEN){
		i = 0;
		do{
			double sm_magnitude = sm_mag[i/8];
			freq_data[(freq_current -2)%Q_HEIGHT][i] |= magnitude_to_sRGB(sm_magnitude) << 8; //shift left by 8 makes green
		}while(++i < FREQ_WIDTH);
	}

	if(enabled_colors & FREQ_RED){
		i = 0;
		do{
			double lg_magnitude = lg_mag[i];
			freq_data[(freq_current -9)%Q_HEIGHT][i] |= magnitude_to_sRGB(lg_magnitude) << 16; //shift left by 16 makes red
		}while(++i < FREQ_WIDTH);
	}
	

	if(enabled_colors & LOGFREQ_GREEN){
		i = 0;
		do{
			double hz = midi_to_hz((i-4.5)/LOGFREQ_KEY_WIDTH);
			unsigned idx = rint(hz*1024.0/48000.0);
			if(idx >= ARRAYSIZE(sm_mag))
				continue;
			double sm_magnitude = sm_mag[idx];
			logfreq_data[(logfreq_current -2)%Q_HEIGHT][i] |= magnitude_to_sRGB(sm_magnitude) << 8; //shift left by 8 makes green
		}while(++i < LOGFREQ_WIDTH);
	}

	if(enabled_colors & LOGFREQ_RED){
		i = 0;
		do{
			double hz = midi_to_hz((i-4.5)/LOGFREQ_KEY_WIDTH);
			unsigned idx = rint(hz*8192.0/48000.0);
			if(idx >= ARRAYSIZE(lg_mag))
				continue;
			double lg_magnitude = lg_mag[idx];
			logfreq_data[(logfreq_current -9)%Q_HEIGHT][i] |= magnitude_to_sRGB(lg_magnitude) << 16; //shift left by 16 makes red
		}while(++i < LOGFREQ_WIDTH);
	}

	display_stuff(freq_win_and_tex, &freq_data[0][0], freq_current%Q_HEIGHT, freq_key_image_height/*FREQ_KEYS_HEIGHT*/);
	display_stuff(logfreq_win_and_tex, &logfreq_data[0][0], logfreq_current%Q_HEIGHT, logfreq_key_image_height/*LOGFREQ_KEYS_HEIGHT*/);
	display_newgraph(newgraph_win_and_tex, msg);

	free(msg); //this is malloced in audio callback function
}


static void display_stuff(window_node *win_node, unsigned *data, unsigned row_index, unsigned key_height){
	if (!win_node) //check if window is closed
		return;
	
//	const unsigned pitch = win_node->width * sizeof (unsigned);

	const unsigned dst_avail = win_node->height - key_height;

	const unsigned recent_src_bottom = row_index;
	const unsigned recent_len = (dst_avail > recent_src_bottom) ? recent_src_bottom : dst_avail;
	const unsigned recent_src_top = recent_src_bottom - recent_len;
	const unsigned recent_dst_top = dst_avail - recent_len;
	const unsigned earlier_src_bottom = Q_HEIGHT;
	const unsigned earlier_len = dst_avail - recent_len;
	const unsigned earlier_src_top = earlier_src_bottom - earlier_len;
	const unsigned earlier_dst_top = recent_dst_top - earlier_len;

	
	if (!win_node->tex_extra){
		win_node->tex_extra = SDL_CreateTexture(
			win_node->rend,
			SDL_PIXELFORMAT_ARGB8888,
			//SDL_TEXTUREACCESS_STATIC, //to use CPU
			SDL_TEXTUREACCESS_STREAMING, //to use GPU
			win_node->width,
			Q_HEIGHT
		);
	}

	int tex_pitch;
	void *tex_pixels;

	//recent and earlier, texture start and length
	unsigned rts = row_index - 9;
	unsigned rtl = 9;
	unsigned ets = row_index - 9;
	unsigned etl = 9;

	if (ets >= Q_HEIGHT){
		ets %= Q_HEIGHT;
		etl = Q_HEIGHT - ets;
		rts = 0;
		rtl -= rts;

		SDL_Rect recent_texrect = (SDL_Rect){.x=0, .y=rts, .w = win_node->width, .h = rtl};
		SDL_LockTexture(win_node->tex_extra, &recent_texrect, &tex_pixels, &tex_pitch);
		memcpy(tex_pixels, data + rts * win_node->width, tex_pitch*rtl);
		SDL_UnlockTexture(win_node->tex_extra);
	}
	SDL_Rect earlier_texrect = (SDL_Rect){.x=0, .y=ets, .w = win_node->width, .h = etl};
	SDL_LockTexture(win_node->tex_extra, &earlier_texrect, &tex_pixels, &tex_pitch);
	memcpy(tex_pixels, data + ets * win_node->width, tex_pitch*etl);
	SDL_UnlockTexture(win_node->tex_extra);
	

	SDL_Rect recent_srcrect = (SDL_Rect){.x = 0,.y = recent_src_top,.w = win_node->width,.h = recent_len};
	SDL_Rect recent_dstrect = (SDL_Rect){.x = 0,.y = recent_dst_top,.w = win_node->width,.h = recent_len};
	SDL_RenderCopy(win_node->rend, win_node->tex_extra, &recent_srcrect, &recent_dstrect);

	SDL_Rect earlier_srcrect = (SDL_Rect){.x = 0,.y = earlier_src_top,.w = win_node->width,.h = earlier_len};
	SDL_Rect earlier_dstrect = (SDL_Rect){.x = 0,.y = earlier_dst_top,.w = win_node->width,.h = earlier_len};
	SDL_RenderCopy(win_node->rend, win_node->tex_extra, &earlier_srcrect, &earlier_dstrect);

	SDL_RenderPresent(win_node->rend);
}


static void display_newgraph(window_node *win_node, struct msg *msg){
	if (!win_node) //check if window is closed
		return;
	
	if (!win_node->tex_extra){
		win_node->tex_extra = SDL_CreateTexture(
			win_node->rend,
			SDL_PIXELFORMAT_ARGB8888,
			//SDL_TEXTUREACCESS_STATIC, //to use CPU
			SDL_TEXTUREACCESS_STREAMING, //to use GPU
			win_node->width,
			Q_HEIGHT
		);
	}

	int tex_pitch;
	void *tex_pixels;



/**/
	const unsigned pitch = win_node->width * sizeof (unsigned);
	unsigned *to_put_in_window = calloc(pitch, win_node->height);

	int i = 0;
	do{
		int j = 0;
		do{
			double s = msg->audio[i*4+j];
			unsigned y = win_node->height - ((s - SAMPLE_MIN) * (double)(win_node->height-1) / (SAMPLE_MAX - SAMPLE_MIN)) - 1;
			to_put_in_window[y * win_node->width + i] = 0xffffff;
		} while (++j < 4);
	} while (++i < 512/4);
/**/



	SDL_Rect recent_texrect = (SDL_Rect){.x=0, .y=0, .w = win_node->width, .h = win_node->height};
	SDL_LockTexture(win_node->tex_extra, &recent_texrect, &tex_pixels, &tex_pitch);
	//memcpy(tex_pixels, data * win_node->width, databytes);
	memcpy(tex_pixels, to_put_in_window, pitch * win_node->height);
	SDL_UnlockTexture(win_node->tex_extra);
	
	if(!msg->audio_chunks_drawn)
		SDL_RenderClear(win_node->rend);

	SDL_Rect recent_srcrect = (SDL_Rect){.x = 0,.y = 0,.w = 512/4,.h = win_node->height};
	SDL_Rect recent_dstrect = (SDL_Rect){.x = 128 * msg->audio_chunks_drawn,.y = 0,.w = 512/4,.h = win_node->height};
	SDL_RenderCopy(win_node->rend, win_node->tex_extra, &recent_srcrect, &recent_dstrect);

	SDL_RenderPresent(win_node->rend);
	free(to_put_in_window);
}

// Function to process a keyboard down event
static void keyboard_down(SDL_KeyboardEvent *event){

	switch(event->keysym.scancode){
	case SDL_SCANCODE_ESCAPE:
		quitting=1;
		break;
	case SDL_SCANCODE_F1: // Set notetype to sawtooth wave
		note_type = 0;
		break;
	case SDL_SCANCODE_F2: // Set note_type to square wave
		note_type = 1;
		break;
	case SDL_SCANCODE_F3: // Set note_type to sine wave
		note_type = 2;
		break;
	case SDL_SCANCODE_F4: // Set note_type to triangle wave
		note_type = 3;
		break;
	case SDL_SCANCODE_INSERT: // Key to toggle red fft graph in freq window
		enabled_colors ^= FREQ_RED;
		break;
	case SDL_SCANCODE_HOME: // Key to toggle green fft graph in freq window
		enabled_colors ^= FREQ_GREEN;
		break;
	case SDL_SCANCODE_PAGEUP: // Key to toggle blue key marks in freq window
		enabled_colors ^= FREQ_BLUE;
		break;
	case SDL_SCANCODE_DELETE: // Key to toggle red fft graph in freqlog window
		enabled_colors ^= LOGFREQ_RED;
		break;
	case SDL_SCANCODE_END: // Key to toggle green fft graph in logfreq window
		enabled_colors ^= LOGFREQ_GREEN;
		break;
	case SDL_SCANCODE_PAGEDOWN: // Key to toggle blue key marks in logfreq window
		enabled_colors ^= LOGFREQ_BLUE;
		break;
	case SDL_SCANCODE_RIGHT: // Inscrease octave 
		if (octave < 7){
			octave++;
			memset(keybits, 0, sizeof keybits);
		}
		break;
	case SDL_SCANCODE_LEFT: // Decrease octave
		if (octave > 1){
			octave--;
			memset(keybits, 0, sizeof keybits);
		}
		break;
	default: // The key down is some key other than the ones listed above
		note_key_done(event->keysym.scancode);
		break;
	}
}


static void handle_event(SDL_Event *event){
	switch(event->type){
	case SDL_WINDOWEVENT:
		if (event->window.event == SDL_WINDOWEVENT_CLOSE){
			closewindow(event->window.windowID);
		}

		if (event->window.event == SDL_WINDOWEVENT_EXPOSED){
			//for some reason this event is called "exposed" and not "maximized"
			//could maybe be some crazy stuff that happens if this event is deltwith for a closed window.
			window_node *tmp = windowID_to_pointer(event->window.windowID);
			if(tmp)
				tmp->expose(tmp);
		}
		break;
	case SDL_KEYDOWN:
		keyboard_down(&event->key);
		break;
	case SDL_KEYUP:
		note_key_up(event->key.keysym.scancode);
		break;
	case SDL_QUIT:
		quitting=1;
		break;
	case SDL_USEREVENT:
		dealwith_userevent(&event->user);
		break;
	default:
		/* do nothing */;
		break;
	}
}


static void rgb24_to_rgb32(int png_width, int png_height, char *rgb24, char *rgb32, const unsigned pitch){
	for (int i = 0; i < png_height; i++){
		unsigned *dst = (unsigned*)rgb32;
		char *source = rgb24;
		for (int j = 0; j < png_width && (char *)dst < pitch + rgb32; j++){
			unsigned char r = *source++;
			unsigned char g = *source++;
			unsigned char b = *source++;
			*dst++ = COLOR(r, g, b);
		}
		
		rgb24 += rgb24_STRIDE;
		rgb32 += pitch;
	}
}


static void mk_win_and_tex(window_node *window, int format){
	window->win = SDL_CreateWindow(
		window->name,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		window->width,
		window->height,
		0
		//SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL
	);

	window->rend = SDL_CreateRenderer(window->win, -1, 0);
	window->tex = SDL_CreateTexture(
		window->rend,
		format,
		// SDL_TEXTUREACCESS_STATIC, // to use CPU
		SDL_TEXTUREACCESS_STREAMING, // to use GPU
		window->width,
		window->height
	);
}


static window_node *spec_and_mk_win_and_tex(window_node *window, char *name, unsigned width, unsigned height, unsigned format){
	window->name = name;
	window->width = width;
	window->height = height;

	mk_win_and_tex(window, format);
	return window;
}

// Function for displaying image windows. A random capability of this program. This program 
// first started as a png to ppm image converter written in python. Later that python program
// was re-written in C. The C version ran MUUUUUCH faster. That C program got turned into an
// image viewer. Later that image viewer later turned into a piano thing.
static void show_img_windowed(window_node *window, char *name){
	window->name = name;
	char *rgb24;
	getprocessed_png(window->name, &window->width, &window->height, &rgb24);
	const unsigned pitch = window->width * sizeof (unsigned);
	char *rgb32 = malloc(pitch * window->height); //this malloc should be done in rgb24_to_rgb32
	rgb24_to_rgb32(window->width, window->height, rgb24, rgb32, pitch);
	mk_win_and_tex(window, SDL_PIXELFORMAT_ARGB8888);
	SDL_UpdateTexture(window->tex, NULL, rgb32, pitch);
	//SDL_RenderClear(window->rend);
	SDL_RenderCopy(window->rend, window->tex, NULL, NULL);
	SDL_RenderPresent(window->rend);

	free(rgb32);
	free(rgb24);
}


static void place_keys_freq(window_node *window){
	// 128.h is a png file converted to C array syntex
	static const unsigned char png_128[] = {
#		include "128.h"
	};
	char *rgb24data;
	unsigned srckeys_width;

	process_png(png_128, sizeof png_128, &srckeys_width, &freq_key_image_height,&rgb24data);

	unsigned *unscaled = calloc(srckeys_width * sizeof *unscaled, freq_key_image_height); //maybe should be done in rgb24_to_rgb_32
	rgb24_to_rgb32(srckeys_width, freq_key_image_height, rgb24data, (char*)unscaled, srckeys_width * sizeof *unscaled);
	free(rgb24data);

	const unsigned texpitch = window->width * sizeof (unsigned);
	unsigned *to_put_in_rect = calloc(texpitch, freq_key_image_height);


	unsigned y = 0;
	do{
		unsigned x = 0;
		do{
			const double midi = hz_to_midi(x*(double)SAMPLE_RATE/8192);
			unsigned dst_x = midi * LOGFREQ_KEY_WIDTH+(LOGFREQ_KEY_WIDTH/2.0)-0.5;
			if (dst_x >= srckeys_width)
				dst_x = srckeys_width - 1;
			
			unsigned pixel = *(unscaled + y*srckeys_width + dst_x);
			*(to_put_in_rect + y*window->width + x) = pixel;
		} while (++x < window->width);
	} while (++y < freq_key_image_height);
	

//	if keysrc.w = srckeys_width, image is scaled when window is wider then image, else image will be black
	SDL_Rect keysrc = (SDL_Rect){.x = 0,.y = 0,.w = window->width,.h = freq_key_image_height};
	SDL_Rect keydst = (SDL_Rect){.x = 0,.y = window->height - freq_key_image_height,.w = window->width,.h = freq_key_image_height};

	if (SDL_UpdateTexture(window->tex, &keysrc, to_put_in_rect, texpitch) == -1)
		puts("Texture not valid");
	if (SDL_RenderCopy(window->rend, window->tex, &keysrc, &keydst))
		puts("error");
	free(unscaled);
	free(to_put_in_rect);
}


//
static void place_keys_logfreq(window_node *window){
	// 128.h is a png file converted to C array syntex
	static const unsigned char png_128[] = {
#		include "128.h"
	};
	char *rgb24data;
	unsigned srckeys_width;

	process_png(png_128, sizeof png_128, &srckeys_width, &logfreq_key_image_height,&rgb24data);

	const unsigned texpitch = window->width * sizeof (unsigned);
	unsigned *to_put_in_rect = calloc(texpitch, logfreq_key_image_height); //maybe should be done in rgb24_to_rgb_32
	rgb24_to_rgb32(srckeys_width, logfreq_key_image_height, rgb24data, (char*)to_put_in_rect, texpitch);
	free(rgb24data);

//	if keysrc.w = srckeys_width, image is scaled when window is wider than image, else image will be black
	SDL_Rect keysrc = (SDL_Rect){.x = 0,.y = 0,.w = window->width,.h = logfreq_key_image_height};
	SDL_Rect keydst = (SDL_Rect){.x = 0,.y = window->height - logfreq_key_image_height,.w = window->width,.h = logfreq_key_image_height};

	if (SDL_UpdateTexture(window->tex, &keysrc, to_put_in_rect, texpitch) == -1)
		puts("Texture not valid");
	if (SDL_RenderCopy(window->rend, window->tex, &keysrc, &keydst))
		puts("error");
	free(to_put_in_rect);
}


////////////////////////////////Object Oriented windows-specific functions////////////////////////////////
// Destroy functions for window, pointers to these functions are handed in when said windows are made.
//
// With closing windows, sometimes special things need to happen on a per window basics. A windows might
// have some special memory that needs to be free'd. Maybe we want a closing animation or sound, or so on.
// Anyway these functions are for making things happen when a window is closed. When a windows is first
// constructed it will be given a pointer to one of these functions.
static void newgraph_win_and_tex_destroy(window_node *node){
	SDL_DestroyTexture(node->tex);
	SDL_DestroyTexture(node->tex_extra);
	newgraph_win_and_tex = NULL;
}

static void freq_win_and_tex_destroy(window_node *node){
	SDL_DestroyTexture(node->tex);
	SDL_DestroyTexture(node->tex_extra);
	freq_win_and_tex = NULL;
}

static void logfreq_win_and_tex_destroy(window_node *node){
	SDL_DestroyTexture(node->tex);
	SDL_DestroyTexture(node->tex_extra);
	logfreq_win_and_tex = NULL;
}


static void dummy_destroy(window_node *node){
	(void)node;
}

// Expose functions for windows. When a new windows is constructed, it is given
// a pointer to one of these expose functions. For windows that don't need anything
// special to happen to be drawn, these windows are give a pointer to the generic 
// function.
static void newgraph_win_and_tex_expose(window_node *node){
	SDL_SetRenderDrawColor(node->rend,  0,0,0,  255);
	SDL_RenderClear(node->rend);
	SDL_RenderPresent(node->rend);
	//display_newgraph(node, audio_for_fft); //pointless, does same as code above
}

static void freq_win_and_tex_expose(window_node *node){
	if (!node) //If window is closed do not mess with it
		return;

	SDL_SetRenderDrawColor(node->rend,  155,80,80,  255);
	SDL_RenderClear(node->rend);
	place_keys_freq(node);
	SDL_RenderPresent(node->rend);
}

static void logfreq_win_and_tex_expose(window_node *node){
	if (!node) //If window is closed do not mess with it
		return;
	
	SDL_SetRenderDrawColor(node->rend,  155,80,80,  255);
	SDL_RenderClear(node->rend);
	place_keys_logfreq(node);
	SDL_RenderPresent(node->rend);
}

static void generic_expose(window_node *node){
	SDL_RenderCopy(node->rend, node->tex, NULL, NULL);
	SDL_RenderPresent(node->rend);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This function runs on the audio thread. The SDL audio thread calls this function
// when it needs more audio to play.
static void audio_callback(void *userdata, Uint8 *stream, int bytes_asked_for){
	(void)userdata;
	const unsigned samples_asked_for = bytes_asked_for / sizeof (sample_type);
	static unsigned audio_chunks_drawn;
	struct msg *msg = malloc(sizeof *msg + bytes_asked_for);
	msg->value = bytes_asked_for;
	memcpy(msg->keybits, keybits, sizeof keybits);
	static unsigned prev_keybits[4];

	if (memcmp(msg->keybits, prev_keybits, sizeof prev_keybits)){
		memcpy(prev_keybits, msg->keybits, sizeof prev_keybits);
		audio_chunks_drawn = 0;
	}else{
		audio_chunks_drawn++;
	}
	msg->audio_chunks_drawn = audio_chunks_drawn;
	
	unsigned s = 0;
	do{
		double sum = 0.0;
		unsigned m = 0;
		do{
			if (!is_midi_playing(m, msg->keybits))
				continue;
			// OLD CODE:
			// const unsigned waves_size = num_samples[note_to_play] * sizeof(sample_type);
			// unsigned where = wave_indexs[note_to_play];
			const unsigned waves_size = num_samples[m];
			unsigned where = wave_indexs[m];
			sum += sample_to_double(waves[note_type][m][where]);
			if (++where == waves_size)
				where = 0;
			wave_indexs[m] = where;
		} while (++m < 128);
		msg->audio[s] = double_to_sample(limiter(sum/4));
	} while (++s < samples_asked_for);
	
	memcpy(stream, msg->audio, bytes_asked_for);


	SDL_Event e;
	e.user.data1 = msg;
	e.user.data2 = (void*)(intptr_t)bytes_asked_for;
	e.type = SDL_USEREVENT;

	SDL_PushEvent(&e);
}


static SDL_AudioSpec wanted={
	.freq = SAMPLE_RATE, // samples per second
	.format = AUDIO_KEVIN, // Format of samples
	.channels = 1, // Number of audio channels, 1 for mono
	.silence = 0,
	.samples = 512, // bits per sample?
	.padding = 0,
	.size = 100000, // Size of something, I do not remember
	.callback = &audio_callback, // Function pointer to function SDL sound thread calls to get more audio
	.userdata = &waves[0][0][0], // I do not remember what this was for
};


static double midi_to_hz(double midi){
	//return = (2^((foo-69)/12))*440hz
	return pow(2,(midi - 69)/12.0)*440/*Hz*/;
}

static double hz_to_midi(double Hz){
	return 12 * log(Hz/440)/M_LN2 + 69;
}

static double waves_that_fit(double freq){
	//subtracted 1 to allow safe rounding
	return (MAX_SAMPLES-1)/(SAMPLE_RATE/freq); //SAMPLE_RATE/freq = samples/wave
}


//usefmod
static void make_sawtooth(sample_type *waves_dst, unsigned samples_per_waves, unsigned wtm){//has sin code
	double waves_to_make = wtm;
	unsigned i = 0;
	double trash;
	do{
		double val = modf(i*waves_to_make/samples_per_waves, &trash);//repeat from 0 to 1
		double adjusted = val * (SAMPLE_MAX - SAMPLE_MIN) + SAMPLE_MIN;
		waves_dst[i] = adjusted; // rounding? Don't go outside range!
	} while (++i < samples_per_waves);
}

static void make_square(sample_type *waves_dst, unsigned samples_per_waves, unsigned wtm){ //has sin code
	double waves_to_make = wtm;
	unsigned i = 0;
	double trash;
	do{
		double val = modf(i*waves_to_make/samples_per_waves, &trash) - 0.5;//repeat from 0 to 1
		double adjusted = signbit(val) ? SAMPLE_MIN : SAMPLE_MAX;
		//double adjusted = signbit(val) ? 0 : 0x7000;
		waves_dst[i] = adjusted; // rounding? Don't go outside range!
	} while (++i < samples_per_waves);
}

static void make_sine(sample_type *waves_dst, unsigned samples_per_waves, unsigned wtm){
	double waves_to_make = wtm;
	unsigned i = 0;
	do{
		double val = sin(i * waves_to_make * 2.0 * M_PI/samples_per_waves);
		//val = val*val*val;  // sine sucks, so cube it and maYBE BETTER?
		double adjusted = double_to_sample(val);
		waves_dst[i] = adjusted; // rounding? Don't go outside range!
	} while (++i < samples_per_waves);
}

static void make_triangle(sample_type *waves_dst, unsigned samples_per_waves, unsigned wtm){//has sin code
	double waves_to_make = wtm;
	unsigned i = 0;
	double trash;
	do{//also us an fabs function
		double val = modf(i*waves_to_make/samples_per_waves, &trash) - 0.5;//repeat from 0 to 1
		double adjusted = 4 * fabs(val) -1;
		waves_dst[i] = double_to_sample(adjusted); // rounding? Don't go outside range!
	} while (++i < samples_per_waves);
}


static void generate_all_waves(void){
	int midi = 0;
	do{
		double samples_per_wave;
		unsigned samples_per_waves;
		unsigned waves_to_make;
		double freq = midi_to_hz(midi);
		samples_per_wave = SAMPLE_RATE/freq;
		waves_to_make = waves_that_fit(freq);
		samples_per_waves = round(waves_to_make * samples_per_wave);
		num_samples[midi] = samples_per_waves;

		make_sawtooth(&waves[0][midi][0], samples_per_waves, waves_to_make);
		make_square(&waves[1][midi][0], samples_per_waves, waves_to_make);
		make_sine(&waves[2][midi][0], samples_per_waves, waves_to_make);
		make_triangle(&waves[3][midi][0], samples_per_waves, waves_to_make);
	}while(++midi<128);
}


int main(int argc, char *argv[]) {
	(void)argc;

	// start SDL
	if(SDL_Init(SDL_INIT_EVERYTHING)<0) {
		printf("Failed SDL_Init %s\n", SDL_GetError());
		return 1;
	}else{
		puts("SDL started");
	}
	

	// generate and start audio:
	puts("generating waves and starting audio");
	generate_all_waves();
	// SDL_OpenAudio(&wanted, &actual); // Letting SDL choose buffer size (etc.) makes things harder
	SDL_OpenAudio(&wanted, NULL); // NULL forces SDL to obey wanted
	SDL_PauseAudio(0);
	puts("done");

	// Do image stuff(Should remove/commit out)
	while (*++argv){
		printf("\n%s:\n", argv[0]);
		show_img_windowed(add_window_node(&dummy_destroy, &generic_expose), argv[0]);
	}
	

	// Window
	newgraph_win_and_tex = spec_and_mk_win_and_tex(add_window_node(&newgraph_win_and_tex_destroy, &newgraph_win_and_tex_expose), "graph", 1234+123, 150, SDL_PIXELFORMAT_ARGB8888);
	freq_win_and_tex = spec_and_mk_win_and_tex(add_window_node(&freq_win_and_tex_destroy, &freq_win_and_tex_expose), "freq", FREQ_WIDTH, LOGFREQ_HEIGHT, SDL_PIXELFORMAT_ARGB8888);
	logfreq_win_and_tex = spec_and_mk_win_and_tex(add_window_node(&logfreq_win_and_tex_destroy, &logfreq_win_and_tex_expose), "logfreq", LOGFREQ_WIDTH, LOGFREQ_HEIGHT, SDL_PIXELFORMAT_ARGB8888);


	// fft
	fft_in = fftw_malloc(sizeof(fftw_complex) * 8192);
	fft_out = fftw_malloc(sizeof(fftw_complex) * 8192);
	fft_sm_plan = fftw_plan_dft_1d(1024, fft_in, fft_out, FFTW_FORWARD, FFTW_ESTIMATE);
	fft_lg_plan = fftw_plan_dft_1d(8192, fft_in, fft_out, FFTW_FORWARD, FFTW_ESTIMATE);

	// Event loop (where program spends most of its time)
	while(!quitting) {
		SDL_Event e;

		//If quiting, will not check events 
		while(!quitting && SDL_WaitEvent(&e)){
			handle_event(&e);
		}
	}
	
	// Quiting stuff
	fftw_destroy_plan(fft_sm_plan);
	fftw_destroy_plan(fft_lg_plan);
	fftw_free(fft_in);
	fftw_free(fft_out);

	SDL_Quit();
	return 0;
}
