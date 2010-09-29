#include <cstdlib>
#include "io.h"
#include "filelist.h"
#include "encodings.h"

using namespace std;

extern bool edit_mode;
int row, col;

namespace
{
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
"\nCD: ",
"\nYear: ",
"\nTrack # ",
"\nTotal tracks: ",
"\nComment: " };

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


void print_INS(const bool ins)
{
	wmove(stat_win, 0, 0);
	if(ins)
		waddstr(stat_win, "INS");
	else
		waddstr(stat_win, "   ");
	wrefresh(stat_win);
}

}


void init_curses()
{
		// basic stuff
		initscr();
		cbreak();
		keypad(stdscr, TRUE);
		nodelay(stdscr, FALSE); // we want a blocking getch()
		noecho();
		getmaxyx(stdscr, row, col);
		check_reso();

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

				for(int i = 0; i < 4; i++)
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


int get_key()
{
	return getch();
}


void redraw_filelist(const bool redraw_everything)
{
	if(redraw_everything)
	{
		wclear(ls_win);
		wattrset(ls_win, COLOR_PAIR(0));
		waddstr(ls_win, directory.c_str());
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
			attr = COLOR_PAIR(int(i->selected)+1); // PAIR(1) or PAIR(2)
			if(i == under_selector)
				attr |= A_STANDOUT;
			wattrset(ls_win, attr);
			if(i->tags.unsaved_changes)
				waddch(ls_win, '*');
			else waddch(ls_win, ' ');
			waddstr(ls_win, i->name.c_str());
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
		if(last_selected != files.end())
		{
			int attr = COLOR_PAIR(3);
			if(edit_mode && idx == idx_to_edit)
				attr |= A_STANDOUT;
			wattrset(tag_win, attr);
			// filename
			if(idx == -1)
			{
				wmove(tag_win, 0, 6); // after "File: "
				waddstr(tag_win, last_selected->info.filename.c_str());
			}
			else
			{
				wmove(tag_win, 3+idx, entry_name[idx].size()-1);
				waddstr(tag_win, last_selected->tags.strs[idx].c_str());
			}
			wclrtoeol(tag_win);
			wrefresh(tag_win);
		}
		else redraw_statics();
}


void redraw_whole_fileinfo()
{
	redraw_statics();

	wmove(tag_win, 1, 0);
	wattrset(tag_win, COLOR_PAIR(0));
	wprintw(tag_win, "brate %d kb/s", last_selected->info.bitrate);
	if(last_selected->info.variable_bitrate)
		wprintw(tag_win, " (VBR)");
	wprintw(tag_win, "\tsrate %d Hz", last_selected->info.samplerate);
	wclrtoeol(tag_win);
	wprintw(tag_win, "\nfsize %d kb\tlen %d:%02d", last_selected->info.size/1024,
		last_selected->info.duration/60, last_selected->info.duration%60);
	wclrtoeol(tag_win);

	for(int i = -1; i < MAX_EDITABLES; ++i)
		redraw_fileinfo(i);
}


string string_editor(const vector<string> &strs, WINDOW *win, const int basex,
	const int basey, const bool append, const bool fixbox)
{
	// the index in the string *in symbols* (so s[n] makes no sense)
	int n = 0;
	vector<string>::const_iterator si = strs.begin();
	string s = *si;
	int N = num_syms(s); // maximum index
	if(append)
		n = N;

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
			waddstr(win, s.c_str());
			wclrtoeol(win);
			if(fixbox) // clrtoeol makes an ugly hole into the box!
			{
				wattrset(win, COLOR_PAIR(0));
				box(win, 0, 0);
			}
			redraw = false;
		}
		wmove(win, basey, basex + n);
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
						redraw = true;
						--n;
						--N;
					}
			}
			else if(key == KEY_DC) // delete key
			{
				if(n < N)
				{
					del(s, n);
					redraw = true;
					--N;
				}
			}
	 		else if(key == KEY_LEFT)
			{
				if(n > 0)
					--n;
			}
			else if(key == KEY_RIGHT)
			{
				if(n < N)
					++n;
			}
			else if(key == KEY_HOME)
				n = 0;
			else if(key == KEY_END)
				n = N;
			else if(key == KEY_IC) // insert key
			{
				insert = !insert;
				redraw = true;
			}
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
				s = *si;
				n = N = num_syms(s);
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
			}
			else // not insert mode; delete and then insert
			{
				if(n < N)
					del(s, n);
				else // typing at the end of the string
					++N;
				ins(s, key, n);
			}
			redraw = true;
			++n;
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
	return s;
}


void edit_tag(const int idx, const bool append, const bool clear)
{
	// get coordinates where the entry begins and a pointer to the content
	int basex, basey;
	string *s;
	if(idx == -1)
	{
		basey = 0;
		basex = 6;
		s = &(last_selected->info.filename);
	}
	else
	{
		basey = 3 + idx;
		basex = entry_name[idx].size()-1;
		s = &(last_selected->tags.strs[idx]);
	}
	string original = *s;
	if(clear)
		s->clear();

	vector<string> strs(1); // an array of one element...
	strs[0] = *s;
	*s = string_editor(strs, tag_win, basex, basey, append);
	redraw_fileinfo(idx);
	if(idx != -1 && original != *s)
	{
		last_selected->need_redraw = last_selected->tags.unsaved_changes = true;
		redraw_filelist();
	}
}



void stat_msg(const char* s)
{
	wmove(stat_win, 0, 4);
	waddstr(stat_win, s);
	wclrtoeol(stat_win);
	wrefresh(stat_win);
}
