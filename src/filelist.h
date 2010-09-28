/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */
#ifndef FILELIST_H
#define FILELIST_H
#include <string>
#include <vector>
#include "fileinfo.h"

struct FilelistEntry
{
	std::string name; // this is the actual name of the file on the hd
	bool selected;
	bool need_redraw;
	FileInfo info;
	Tag tags;

	FilelistEntry(const std::string n, const FileInfo fi, const Tag t)
		: name(n), selected(false), need_redraw(true), info(fi), tags(t) {}
};

extern std::string directory;
extern std::vector<FilelistEntry> files;
extern std::vector<FilelistEntry>::iterator under_selector;
extern std::vector<FilelistEntry>::iterator last_selected;

bool get_directory(const char* name);

// those of these that return something return whether the info entry should update
void toggle_select(); // toggle selection of entry under selector
bool select_or_show(); // set entry selected
void deselect_all();
bool select_down(); // from selector
bool select_up();
void goto_begin();
void goto_end();
void move_up();
void move_down();
void invert_selection();

// returns message to be printed
std::string write_modifieds();

#endif
