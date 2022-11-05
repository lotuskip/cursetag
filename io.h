#include <ncurses.h>

void init_curses(void);
void deinit_curses(void);
void redraw_filelist(int redraw_everything);
void redraw_fileinfo(int idx);
void redraw_whole_fileinfo(void);
char* string_editor(const char **strs, int n_strs, WINDOW *win, int basex, int basey,
	int append, int boxsize, int fixbox);
void edit_tag(int idx, int append, int clear);
void stat_msg(const char* s);
