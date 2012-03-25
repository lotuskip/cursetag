#include <cstdlib>
#include <cstring>
#include "io.h"
#include "filelist.h"
#include "encodings.h"

using namespace std;

extern bool edit_mode; // defined in inputhandle.cpp
int row, col;

int fname_print_pos = 0;

namespace
{
const char OUR_ESC_DELAY = 20; //ms

WINDOW *ls_win; // file list
WINDOW *tag_win; // tags of selected file
WINDOW *stat_win; // "statusbar"

vector<FilelistEntry>::const_iterator draw_rng_begin, draw_rng_end;

void check_reso() // throws!
{
	if(row < MIN_ROWS || col < MIN_COLS)
		throw 1;
}

const string entry_name[MAX_EDITABLES] = {
"Title:  ",
"\nArtist: ",
"\nAlbum: ",
"\nYear: ",
"\nTrack: ",
"\nComment: " };

const char* ins_str[2] = { "   ", "INS" };


void print_INS(const bool ins)
{
	wmove(stat_win, 0, 0);
	waddstr(stat_win, ins_str[ins]);
	wrefresh(stat_win);
}


// Print as much as possible and add a '>' in different
// colour if there is more to print.
void print_amap(const string &s, const int x, const int y, const int boxsize)
{
	wmove(tag_win, y, x);
	if(s.size() > boxsize)
	{
		waddstr(tag_win, mb_substr(s, 0, boxsize-1));
		wattrset(tag_win, COLOR_PAIR(2));
		waddch(tag_win, '>');
	}
	else
	{
		waddstr(tag_win, s.c_str());
		wclrtoeol(tag_win);
	}
}


// Functionality to write a filesize prettily:
const char *sizes[] = { "Tb", "Gb", "Mb", "kb", "b" };
char printable_size[10]; // might not be enough if file size > 100 Tb

void produce_print_size(const unsigned long size)
{
	unsigned long multiplier = 1024UL*1024UL*1024UL*1024UL; // 1 Tb

	for(int i = 0; i < 5; ++i, multiplier /= 1024)
	{
		if(size < multiplier)
			continue;
		if(!(size % multiplier))
			snprintf(printable_size, sizeof(printable_size),
				"%u %s", size/multiplier, sizes[i]);
		else
			snprintf(printable_size, sizeof(printable_size),
				"%.1f %s", float(size)/multiplier, sizes[i]);
		return;
	}
	strcpy(printable_size, "0");
}

} // end local namespace


void init_curses()
{
	// basic stuff
	initscr();
	getmaxyx(stdscr, row, col);
	check_reso(); // might throw!
	cbreak();
	keypad(stdscr, TRUE);
	nodelay(stdscr, FALSE); // we want a blocking getch()
	noecho();
	curs_set(0);
	set_escdelay(OUR_ESC_DELAY);

	// create some windows
	ls_win = newwin(row-1, col/2, 0, 0);
	tag_win = newwin(row-1, col/2, 0, col/2);
	stat_win = newwin(1, col, row-2, 0);

	// colours stuff
	if(has_colors())
	{
		start_color();
		char ct[4] = { COLOR_WHITE, // hard-wired; can't be changed; used for static texts
		COLOR_CYAN, // unselected file in file list
		COLOR_BLUE, // selected file in filelist
		COLOR_GREEN }; //modifiable field in file info

		for(int i = 0; i < 4; ++i)
			init_pair(i, ct[i], COLOR_BLACK); // background
	}

	draw_rng_begin = files.begin();
	if((signed int)files.size() <= row-4)
		draw_rng_end = files.end();
	else
		draw_rng_end = draw_rng_begin + (row-4);
	refresh();
	redraw_statics();
	redraw_filelist(true);
}


void deinit_curses()
{
	delwin(ls_win);
	delwin(tag_win);
	delwin(stat_win);
	echo();
	endwin();
}


bool update_reso()
{
	int oldr = row, oldc = col;
	refresh();
	getmaxyx(stdscr, row, col);

	if(oldr != row || oldc != col)
	{
		check_reso(); // might throw!
		delwin(ls_win);
		delwin(tag_win);
		delwin(stat_win);
		ls_win = newwin(row-1, col/2, 0, 0);
		tag_win = newwin(row-1, col/2, 0, col/2);
		stat_win = newwin(1, col, row-1, 0);

		if((signed int)files.size() > row-4) // all files do not fit on the list!
		{
			// get optimal range to view so that selected entry is about in the middle:
			draw_rng_begin = draw_rng_end = under_selector;
			int num = 0;
			while(num < row-4)
			{
				if(draw_rng_begin != files.begin())
				{
					--draw_rng_begin;
					if(++num == row-4)
						break;
				}
				if(draw_rng_end != files.end())
				{
					++draw_rng_end;
					++num;
				}
			}
		}
		else // all fit; might be that didn't fit previously!
		{
			draw_rng_begin = files.begin();
			draw_rng_end = files.end();
		}

		// changes took place; redraw everything:
		redraw_filelist(true);
		redraw_whole_fileinfo();
		return true;
	}
	return false;
}


int get_key() { return getch(); }


void redraw_statics()
{
	wclear(tag_win);
	wattrset(tag_win, COLOR_PAIR(0));
	waddstr(tag_win, "File: ");
	wclrtoeol(tag_win);
	wmove(tag_win, 3, 0);
	for(int i = 0; i < MAX_EDITABLES; ++i)
	{
		wattrset(tag_win, COLOR_PAIR(0));
		waddstr(tag_win, entry_name[i].c_str());
		wclrtoeol(tag_win);
	}
	wrefresh(tag_win);
}


void redraw_filelist(const bool redraw_everything) // def: false
{
	bool fixbeg = (fname_print_pos > 0);
	bool fixend;
	int syms;

	if(redraw_everything)
	{
		wclear(ls_win);
		if(fixbeg)
		{
			wattrset(ls_win, COLOR_PAIR(2));
			waddch(ls_win, '<');
		}
		syms = num_syms(directory);
		fixend = (syms - fname_print_pos - fixbeg > col/2);
		if(syms > fname_print_pos)
		{
			wattrset(ls_win, COLOR_PAIR(3));
			waddstr(ls_win, mb_substr(directory, fname_print_pos,
				col/2 - fixbeg - fixend));
		}
		if(fixend)
		{
			wattrset(ls_win, COLOR_PAIR(2));
			waddch(ls_win, '>');
		}
	}
	else
	{
		// check if need to modify range:
		while(under_selector < draw_rng_begin)
		{
			--draw_rng_begin;
			--draw_rng_end;
		}
		while(under_selector >= draw_rng_end)
		{
			++draw_rng_begin;
			++draw_rng_end;
		}
	}
	// Print actual file entries:
	wmove(ls_win, 1, 0);
	int attr, y;
	if(draw_rng_begin != files.begin())
	{
		wattrset(ls_win, COLOR_PAIR(0));
		waddstr(ls_win, "[^]\n");
	}
	else { wclrtoeol(ls_win); wmove(ls_win, 2, 0); }

	for(vector<FilelistEntry>::const_iterator i = draw_rng_begin; i != draw_rng_end; ++i)
	{
		if(redraw_everything || i->need_redraw)
		{
			syms = num_syms(i->name);
			// The "-1" here comes from the possible '*' indicator
			fixend = (syms - fname_print_pos - 1 > col/2);
			attr = COLOR_PAIR(int(i->selected)+1); // PAIR(1) or PAIR(2)
			if(i == under_selector)
				attr |= A_STANDOUT;
			wattrset(ls_win, attr);
			if(i->tags.unsaved_changes)
				waddch(ls_win, '*');
			else waddch(ls_win, ' ');
			if(syms > fname_print_pos)
				waddstr(ls_win, mb_substr(i->name, fname_print_pos,
					col/2 - fixend - 1));
			if(fixend)
			{
				wattrset(ls_win, COLOR_PAIR(2));
				waddch(ls_win, '>');
			}
			else if(syms - fname_print_pos + 1 < col/2)
				waddch(ls_win, '\n');
		}
		else
		{
			getyx(ls_win, y, attr);
			wmove(ls_win, y+1, 0);
		}
	}
	if(draw_rng_end != files.end())
	{
		wattrset(ls_win, COLOR_PAIR(0));
		waddstr(ls_win, "[v]\n");
	}
	else wclrtoeol(ls_win);
	wrefresh(ls_win);
}


void redraw_fileinfo(const int idx)
{
	string* thestr = (idx == -1 ? &(last_selected->info.filename)
		: &(last_selected->tags.strs[idx]));
	int attr = COLOR_PAIR(3);
	if(edit_mode && idx == idx_to_edit)
	{
		attr |= A_STANDOUT;
		if(thestr->empty())
			curs_set(1);
		else
			curs_set(0);
	}
	wattrset(tag_win, attr);
	if(idx == -1) // filename
		print_amap(*thestr, 6, 0, col/2-6); // 6=len("file: ")
	else
		print_amap(*thestr, entry_name[idx].size()-1, 3+idx, col/2-entry_name[idx].size()+1);
	wrefresh(tag_win);
}


void redraw_whole_fileinfo()
{
	redraw_statics();

	wmove(tag_win, 1, 0);
	wattrset(tag_win, COLOR_PAIR(0));
	wprintw(tag_win, "brate %d kb/s", last_selected->info.bitrate);
	wprintw(tag_win, "\tsrate %d Hz", last_selected->info.samplerate);
	wclrtoeol(tag_win);

	produce_print_size(last_selected->info.size);
	wprintw(tag_win, "\nfsize %-9s\tlen %d:%02d", printable_size,
		last_selected->info.duration/60, last_selected->info.duration%60);
	wclrtoeol(tag_win);

	if(last_selected != files.end())
	{
		for(int i = -1; i < MAX_EDITABLES; ++i)
			redraw_fileinfo(i);
		// return cursor to proper location (needed in case entry under cursor
		// is empty)
		if(idx_to_edit == -1) // filename
			wmove(tag_win, 0, 6); // 6=len("file: ")
		else
			wmove(tag_win, 3+idx_to_edit, entry_name[idx_to_edit].size()-1);
		wrefresh(tag_win);
	}
}


string string_editor(const vector<string> &strs, WINDOW *win, const int basex,
	const int basey, const bool append, const int boxsize, const bool fixbox)
{
	curs_set(1);

	// the index in the string *in UTF-8 symbols* (so s[n] makes no sense)
	int n = 0;
	// The index at which we start to print onto screen:
	int printb_pos = 0;

	vector<string>::const_iterator si = strs.begin();
	string s = *si;
	int N = num_syms(s); // maximum index
	if(append)
	{
		n = N;
		printb_pos = max(0, N - boxsize + 1);
	}

	int k;
	wint_t key;
	bool redraw = true;
	bool insert = true;
	bool replace_with = false;
	string original = s;
	try {
	for(;;)
	{
		if(redraw)
		{
			print_INS(insert);
			wmove(win, basey, basex);
			wattrset(win, COLOR_PAIR(3)|A_UNDERLINE);
			waddstr(win, mb_substr(s, printb_pos, min(N - printb_pos, boxsize)));
			if(N - printb_pos < boxsize)
			{
				wclrtoeol(win);
				if(fixbox) // clrtoeol makes an ugly hole into the box!
				{
					wattrset(win, COLOR_PAIR(0));
					box(win, 0, 0);
				}
			}
			redraw = false;
		}
		wmove(win, basey, basex + n - printb_pos);
		wrefresh(win);

		if((k = get_wch(&key)) == KEY_CODE_YES)
		{
			if(key == KEY_RESIZE)
			{
				if(update_reso())
					break; // users shouldn't be resizing during editing
			}
			else if(key == KEY_ENTER) // done (note that meaning of KEY_ENTER is somewhat in the air... Need to check for '\n', too)
				break;
			// else
			if(key == KEY_BACKSPACE)
			{
				if(n > 0)
				{
					del(s, n-1);
					if(--n < printb_pos)
						printb_pos = max(0, printb_pos - boxsize/2);
					--N;
					redraw = true;
				}
			}
			else if(key == KEY_DC) // delete key
			{
				if(n < N)
				{
					del(s, n);
					--N;
					redraw = true;
				}
			}
	 		else if(key == KEY_LEFT)
			{
				if(n > 0)
				{
					if(--n < printb_pos)
					{
						printb_pos = max(0, printb_pos - boxsize/2);
						redraw = true;
					}
				}
			}
			else if(key == KEY_RIGHT)
			{
				if(n < N)
				{
					if(++n >= printb_pos + boxsize)
					{
						++printb_pos;
						redraw = true;
					}
				}
			}
			else if(key == KEY_HOME)
			{
				n = printb_pos = 0;
				redraw = true;
			}
			else if(key == KEY_END)
			{
				n = N;
				printb_pos = max(0, N - boxsize + 1);
				redraw = true;
			}
			else if(key == KEY_IC) // insert key
				print_INS((insert = !insert));
			else if(key == KEY_UP)
			{
				if(si != strs.begin())
					--si;
				else
					si = strs.end()-1;
				replace_with = true;
			}
			else if(key == KEY_DOWN)
			{
				if(si != strs.end()-1)
					++si;
				else
					si = strs.begin();
				replace_with = true;
			}
			
			if(replace_with)
			{
				n = N = num_syms((s = *si));
				printb_pos = max(0, N - boxsize + 1);
				redraw = true;
				replace_with = false;
			}
		}
		else if(k == OK) // key holds a proper wide character
		{
			if(key == '\n') // the alternate "done"
				break;
			if(key == 27) // escape
			{
				s = original;
				break;
			}
			// else
			if(insert)
			{
				ins(s, key, n);
				++N; // string got longer
				++n;
				if(n >= printb_pos + boxsize)
					++printb_pos;
			}
			else // not insert mode; delete and then insert
			{
				if(n < N)
					del(s, n);
				else // typing at the end of the string
					++N;
				ins(s, key, n);
				++n;
				if(n >= printb_pos + boxsize)
					++printb_pos;
			}
			redraw = true;
		}
		else //i == ERR (? perhaps resize event?)
		{
			if(update_reso())
				break; // users shouldn't be resizing during editing
		}
	} // for(;;)
	}
	catch(utf_error e)
	{
		stat_msg("WARNING; problem handling the UTF-8 string! Cancelling changes!");
		s = original;
	}
	//DONE!
	print_INS(false); // remove any "INS"
	curs_set(0);
	return s;
}


void edit_tag(const int idx, const bool append, const bool clear)
{
	// get coordinates where the entry begins and a pointer to the content
	int basex, basey, boxsize;
	string *s;
	if(idx == -1)
	{
		basey = 0;
		basex = 6;
		boxsize = col/2 - 6; // 6 = len("File: ")
		s = &(last_selected->info.filename);
	}
	else
	{
		basey = 3 + idx;
		basex = entry_name[idx].size()-1;
		boxsize = col/2 - basex;
		s = &(last_selected->tags.strs[idx]);
	}
	string original = *s;
	if(clear)
		s->clear();

	vector<string> strs(1); // an array of one element...
	strs[0] = *s;
	*s = string_editor(strs, tag_win, basex, basey, append, boxsize);
	//redraw_fileinfo(idx);
	if(idx != -1 && original != *s)
	{
		last_selected->need_redraw = last_selected->tags.unsaved_changes = true;
		redraw_filelist();
	}
	stat_msg("");
}


void stat_msg(const char* s)
{
	wmove(stat_win, 0, 4);
	waddstr(stat_win, s);
	wclrtoeol(stat_win);
	wrefresh(stat_win);
}

