#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <wchar.h>
#include "io.h"
#include "config.h"
#include "utils.h"
#include "filelist.h"

extern int edit_mode;
int row, col;
int fname_print_pos = 0;

static WINDOW *ls_win; /* file list */
static WINDOW *tag_win; /* tags of selected file */
static WINDOW *stat_win; /* "statusbar" */

static int draw_rng_begin, draw_rng_len;

static const char *const entry_name[MAX_EDITABLES] = {
	"Title:  ",
	"\nArtist: ",
	"\nAlbum: ",
	"\nYear: ",
	"\nTrack: ",
	"\nComment: " };

static const char *const ins_str[] = { "   ", "INS" };

static void print_INS(const int ins)
{
	wmove(stat_win, 0, 0);
	waddstr(stat_win, ins_str[ins]);
	wrefresh(stat_win);
}

/* "Print as much as possible" */
static void print_amap(const char *const s, const int x, const int y, const int boxsize)
{
	wmove(tag_win, y, x);
	waddnstr(tag_win, s, boxsize-1);
	if (num_syms(s) > boxsize) {
		wattrset(tag_win, COLOR_PAIR(3));
		waddch(tag_win, '>');
	} else {
		wclrtoeol(tag_win);
	}
}

static char print_size[16];
static void produce_print_size(const off_t size)
{
	const char pfix[] = "BKMGTPE";
	unsigned int i = 0;
	double n;
	for (n = size; n >= 1024 && i < strlen(pfix); ++i)
		n /= 1024;
	if (i)
		snprintf(print_size, sizeof(print_size), "%.1f%c", n, pfix[i]);
	else
		snprintf(print_size, sizeof(print_size), "%ju", (uintmax_t)size);
}

static void redraw_statics(void)
{
	wclear(tag_win);
	wattrset(tag_win, COLOR_PAIR(1));
	waddstr(tag_win, "File: ");
	wclrtoeol(tag_win);
	wmove(tag_win, 3, 0);
	for (int i = 0; i < MAX_EDITABLES; ++i) {
		wattrset(tag_win, COLOR_PAIR(1));
		waddstr(tag_win, entry_name[i]);
		wclrtoeol(tag_win);
	}
	wrefresh(tag_win);
}

#define MIN_COLS 60
#define MIN_ROWS 10
void init_curses(void)
{
	int i;
	initscr();
	getmaxyx(stdscr, row, col);
	if (row < MIN_ROWS || col < MIN_COLS) {
		fprintf(stderr, "Terminal too small!\n(needed at least %ix%i)\n", MIN_COLS, MIN_ROWS);
		exit(1);
	}
	cbreak();
	keypad(stdscr, TRUE);
	nodelay(stdscr, FALSE);
	noecho();
	curs_set(0);
	set_escdelay(OUR_ESC_DELAY);
	ls_win = newwin(row-1, col/2, 0, 0);
	tag_win = newwin(row-1, col/2, 0, col/2);
	stat_win = newwin(1, col, row-2, 0);
	if (has_colors()) {
		start_color();
		for (i = 0; i < 5; ++i)
			init_pair(i, foreground[i], background);
	}
	refresh();
	draw_rng_begin = 0;
	draw_rng_len = min(n_files, row-4);
	redraw_statics();
	redraw_filelist(1);
}

void deinit_curses(void)
{
	delwin(ls_win);
	delwin(tag_win);
	delwin(stat_win);
	echo();
	endwin();
}

static void print_beg_n(WINDOW *win, const char *s, const int beg, const int n)
{
	const char* p = s + mb_substr(s,beg);
	waddnstr(win, p, mb_substr(p, n));
}

void redraw_filelist(const int redraw_everything)
{
	int fixbeg = (fname_print_pos > 0);
	int fixend, syms, attr, y, k;

	if (redraw_everything) {
		wclear(ls_win);
		if (fixbeg) {
			wattrset(ls_win, COLOR_PAIR(3));
			waddch(ls_win, '<');
		}
		syms = num_syms(directory);
		fixend = (syms + fixbeg - fname_print_pos > col/2);
		if (syms > fname_print_pos) {
			wattrset(ls_win, COLOR_PAIR(4));
			print_beg_n(ls_win, directory, fname_print_pos + fixbeg, col/2 - fixbeg - fixend);
		}
		if (fixend) {
			wattrset(ls_win, COLOR_PAIR(3));
			waddch(ls_win, '>');
		}
	} else if (under_sel < draw_rng_begin) {
		draw_rng_begin = under_sel;
	} else if (under_sel >= draw_rng_begin + draw_rng_len) {
		draw_rng_begin = under_sel + 1 - draw_rng_len;
	}

	wmove(ls_win, 1, 0);
	if (draw_rng_begin) {
		wattrset(ls_win, COLOR_PAIR(1));
		waddstr(ls_win, "[^]\n");
	} else {
		wclrtoeol(ls_win);
		wmove(ls_win, 2, 0);
	}
	for (k = draw_rng_begin; k < draw_rng_begin+draw_rng_len; ++k) {
		if (redraw_everything || files[k]->need_redraw) {
			if (fixbeg) {
				wattrset(ls_win, COLOR_PAIR(3));
				waddch(ls_win, '<');
			}
			syms = num_syms(files[k]->name);
			/* The "-1" here comes from the possible '*' indicator */
			fixend = (syms - fname_print_pos > col/2 - 1 - fixbeg);
			attr = COLOR_PAIR(files[k]->selected+2); /* PAIR(2) or PAIR(3) */
			if (k == under_sel)
				attr |= A_STANDOUT;
			wattrset(ls_win, attr);
			waddch(ls_win, files[k]->tags.unsaved_changes ? '*' : ' ');
			if (syms + 1 + fixbeg > fname_print_pos)
				print_beg_n(ls_win, files[k]->name, fname_print_pos + fixbeg, col/2 - fixbeg - fixend - 1);
			if (fixend) {
				wattrset(ls_win, COLOR_PAIR(3));
				waddch(ls_win, '>');
			} else if (syms + 1 - fname_print_pos < col/2) {
				waddch(ls_win, '\n');
			}
		} else {
			getyx(ls_win, y, attr);
			wmove(ls_win, y+1, 0);
		}
	}
	if (draw_rng_begin + draw_rng_len < n_files) {
		wattrset(ls_win, COLOR_PAIR(1));
		waddstr(ls_win, "[v]\n");
	} else {
		wclrtoeol(ls_win);
	}
	wrefresh(ls_win);
}

void redraw_fileinfo(const int idx)
{
	if (last_sel == n_files)
		return;

	char* thestr = (idx == -1 ? files[last_sel]->info.filename
		: files[last_sel]->tags.strs[idx]);
	int attr = COLOR_PAIR(4);
	if (edit_mode && idx == edit_idx) {
		attr |= A_STANDOUT;
		curs_set(!thestr[0]);
	}
	wattrset(tag_win, attr);
	if (idx == -1) /* filename */
		print_amap(thestr, 6, 0, col/2-6); /* 6=len("file: ") */
	else
		print_amap(thestr, strlen(entry_name[idx])-1, 3+idx,
			col/2-strlen(entry_name[idx])+1);
	wrefresh(tag_win);
}

void redraw_whole_fileinfo(void)
{
	redraw_statics();
	if (last_sel == n_files)
		return;

	wmove(tag_win, 1, 0);
	wattrset(tag_win, COLOR_PAIR(1));
	wprintw(tag_win, "brate %d kb/s", files[last_sel]->info.bitrate);
	wprintw(tag_win, "\tsrate %d Hz", files[last_sel]->info.samplerate);
	wclrtoeol(tag_win);

	produce_print_size(files[last_sel]->info.size);
	wprintw(tag_win, "\nfsize %-9s\tlen %d:%02d", print_size,
		files[last_sel]->info.duration/60, files[last_sel]->info.duration%60);
	wclrtoeol(tag_win);

	for (int i = -1; i < MAX_EDITABLES; redraw_fileinfo(i++));
	/* return cursor to proper location (needed in case entry under cursor is empty) */
	if (edit_idx == -1)
		wmove(tag_win, 0, 6); /* 6=len("file: ") */
	else
		wmove(tag_win, 3+edit_idx, strlen(entry_name[edit_idx])-1);
	wrefresh(tag_win);
}

char edit_buf[BUF_LEN];
char* string_editor(const char **strs, const int n_strs, WINDOW *win, const int basex,
	const int basey, const int append, const int boxsize, const int fixbox)
{

	int k, redraw = 1, insert = 1, replace_with = 0;
	wint_t key;
	int n = 0; /* the index in the string in symbols */
	int printb_pos = 0; /* The index at which we start to print onto screen */
	int str_i = 0; /* index to 'strs' array */
	int N = num_syms(strs[0]); /* upper limit for 'n' */
	if (append) {
		n = N;
		printb_pos = max(0, N - boxsize + 1);
	}

	curs_set(1);
	for (strcpy(edit_buf, strs[0]);;) {
		if (redraw) {
			print_INS(insert);
			wmove(win, basey, basex);
			wattrset(win, COLOR_PAIR(4)|A_UNDERLINE);
			print_beg_n(win, edit_buf, printb_pos, min(N - printb_pos, boxsize));
			if (N - printb_pos < boxsize) {
				wclrtoeol(win);
				if (fixbox) { /* clrtoeol makes an ugly hole into the box */
					wattrset(win, COLOR_PAIR(1));
					box(win, 0, 0);
				}
			}
			redraw = 0;
		}
		wmove(win, basey, basex + n - printb_pos);
		wrefresh(win);

		if ((k = get_wch(&key)) == KEY_CODE_YES) {
			if (key == KEY_ENTER) /* done (note that meaning of KEY_ENTER is somewhat in the air... Need to check for '\n', too) */
				break;
			if (key == KEY_BACKSPACE) {
				if (n > 0) {
					del(edit_buf, n-1);
					if (--n < printb_pos)
						printb_pos = max(0, printb_pos - boxsize/2);
					--N;
					redraw = 1;
				}
			} else if (key == KEY_DC) {
				if (n < N) {
					del(edit_buf, n);
					--N;
					redraw = 1;
				}
			} else if (key == KEY_LEFT) {
				if (n > 0) {
					if (--n < printb_pos) {
						printb_pos = max(0, printb_pos - boxsize/2);
						redraw = 1;
					}
				}
			} else if (key == KEY_RIGHT) {
				if (n < N) {
					if (++n >= printb_pos + boxsize) {
						++printb_pos;
						redraw = 1;
					}
				}
			} else if (key == KEY_HOME) {
				n = printb_pos = 0;
				redraw = 1;
			} else if (key == KEY_END) {
				n = N;
				printb_pos = max(0, N - boxsize + 1);
				redraw = 1;
			} else if (key == KEY_IC) {
				print_INS((insert = !insert));
			} else if (key == KEY_UP) {
				if (str_i)
					--str_i;
				else
					str_i = n_strs-1;
				replace_with = 1;
			} else if (key == KEY_DOWN) {
				if (str_i != n_strs-1)
					++str_i;
				else
					str_i = 0;
				replace_with = 1;
			}
			
			if (replace_with) {
				n = N = num_syms(strcpy(edit_buf, strs[str_i]));
				printb_pos = max(0, N - boxsize + 1);
				redraw = 1;
				replace_with = 0;
			}
		} else if (k == OK) {
			if (key == '\n')
				break;
			if (key == 27) { /* escape */
				strcpy(edit_buf, strs[0]);
				break;
			} else if (insert) {
				N += ins(edit_buf, key, n);
			} else {
				if (n < N) {
					del(edit_buf, n);
					--N;
				}
				N += ins(edit_buf, key, n);
			}
			if (++n >= printb_pos + boxsize)
				++printb_pos;
			redraw = 1;
		}
	}
	print_INS(0);
	curs_set(0);
	return edit_buf;
}

void edit_tag(const int idx, const int append, const int clear)
{
	int basex, basey, boxsize;
	char **sp;
	char s[BUF_LEN] = "";

	if (idx == -1) {
		basey = 0;
		basex = 6; /* = len("File: ") */
		sp = &(files[last_sel]->info.filename);
	} else {
		basey = 3 + idx;
		basex = strlen(entry_name[idx])-1;
		sp = &(files[last_sel]->tags.strs[idx]);
	}
	boxsize = col/2 - basex;

	if (!clear)
		strcpy(s, *sp);

	const char *array = s;
	string_editor(&array, 1, tag_win, basex, basey, append, boxsize, 0);
	if (strcmp(edit_buf, *sp)) {
		realloc_cp(sp, edit_buf);
		files[last_sel]->need_redraw = files[last_sel]->tags.unsaved_changes = 1;
		redraw_filelist(0);
	}
	stat_msg("");
}

void stat_msg(const char *const s)
{
	wmove(stat_win, 0, 4);
	wattrset(stat_win, COLOR_PAIR(1));
	waddstr(stat_win, s);
	wclrtoeol(stat_win);
	wrefresh(stat_win);
}
