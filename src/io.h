#ifndef IO_H
#define IO_H
#include <string>
#include <vector>
#include <ncurses.h>

const int MIN_COLS = 60, MIN_ROWS = 15;

void init_curses();
void deinit_curses();
bool update_reso(); // called when terminal size is changed
int get_key();
void redraw_filelist(const bool redraw_everything = false);
void redraw_fileinfo(const int idx);
void redraw_whole_fileinfo();
std::string string_editor(const std::vector<std::string> &strs, WINDOW *win, const int basex,
	const int basey, const bool append, const bool fixbox = false);
void edit_tag(const int idx, const bool append, const bool clear = false);
void stat_msg(const char* s);

#endif

