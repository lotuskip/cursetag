/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */
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
std::string string_editor(std::vector<std::string> strs, WINDOW *win, const int basex, const int basey, const bool append);
void edit_tag(const int idx, const bool append, const bool clear = false);
void stat_msg(const char* s);

#endif

