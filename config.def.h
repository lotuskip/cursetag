/* To change the colours in interactive mode. Just the basic NCurses 8:
 * COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA,
 * COLOR_CYAN, COLOR_WHITE.
 * Please understand how NCurses colours work and how they depend on the used
 * terminal and its settings. */

const char foreground[5] = { COLOR_WHITE,
	COLOR_WHITE, /* static texts */
	COLOR_CYAN, /* unselected file in file list */
	COLOR_BLUE, /* selected file in filelist */
	COLOR_GREEN }; /* modifiable field in file info */

#define background COLOR_BLACK

/* You can also change this if you like. */
#define OUR_ESC_DELAY 20 /*ms*/
