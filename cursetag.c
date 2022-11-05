#include <locale.h>
#include <stdio.h>
#include <string.h>
#include "io.h"
#include "utils.h"
#include "filelist.h"

int edit_mode = 0;

extern int fname_print_pos;
extern int col;

int rename_selected(const char *pattern);
int fill_selected(const char *pattern);

static void leave_editmode(void)
{
	edit_mode = 0;
	redraw_fileinfo(edit_idx);
	stat_msg("Left edit mode.");
}

static void clear_and_do(void (*const f)(void))
{
	stat_msg(""); f(); redraw_filelist(0);
}

static int any_unsaved(void)
{
	int k;

	for (k = 0; k < n_files && !files[k]->tags.unsaved_changes; ++k);
	return (k < n_files);
}

static void update_all(const char* msg)
{
	redraw_whole_fileinfo();
	redraw_filelist(0);
	if (msg) {
		if (edit_mode)
			leave_editmode();
		else
			stat_msg(msg);
	}
}

static int need_sel(const char *msg)
{
	if (last_sel != n_files)
		return 1;
	stat_msg(msg);
	return 0;
}

static void mainloop(void)
{
	int k, changed;
	char tmp[80];

	for (;;) {
		/*
		 * Global keys
		 */
		if ((k = getch()) == 'Q') {
			if (any_unsaved()) {
				stat_msg("There are unsaved changes! Enter '!' to confirm quit or anything else to cancel.");
				if (getch() != '!')
					continue;
			}
			break;
		} else if (k == 'h' || k == KEY_LEFT) {
			if (fname_print_pos > 0) {
				--fname_print_pos;
				redraw_filelist(1);
			}
		} else if (k == 'l' || k == KEY_RIGHT) {
			if (fname_print_pos < longest_fname_len + 1 - col/2) {
				++fname_print_pos;
				redraw_filelist(1);
			}
		} else if (k == 'A') {
			stat_msg("All files selected.");
			select_all();
			update_all(NULL);
		} else if (k == 'I') {
			invert_selection();
			update_all("Inverse selection.");
		} else if (k == 'V') {
			deselect_all();
			update_all("Selection cleared.");
		} else if (k == '>') {
			stat_msg("");
			move_down();
			if (select_or_show())
				update_all(NULL);
		} else if (k == '<') {
			stat_msg("");
			move_up();
			if (select_or_show())
				update_all(NULL);
		} else if (k == 'J') {
			clear_and_do(select_down);
		} else if (k == 'K') {
			clear_and_do(select_up);
		}
		/*
		 * Edit-mode keys
		 */
		else if (edit_mode) {
			switch(k) {
			case KEY_UP: case 'k':
				if (edit_idx > -1) {
					stat_msg("");
					redraw_fileinfo(edit_idx--);
					redraw_fileinfo(edit_idx);
				}
				break;
			case KEY_DOWN: case 'j':
				if (edit_idx < MAX_EDITABLES-1) {
					stat_msg("");
					redraw_fileinfo(edit_idx++);
					redraw_fileinfo(edit_idx);
				}
				break;
			case 27: /* escape */
			case '!':
			case '\t':
				leave_editmode();
				break;
			case 'g': case KEY_HOME:
				stat_msg("First file.");
				goto_begin();
				if (select_or_show())
					redraw_whole_fileinfo();
				redraw_filelist(0);
				break;
			case 'G': case KEY_END:
				stat_msg("Last file.");
				goto_end();
				if (select_or_show())
					redraw_whole_fileinfo();
				redraw_filelist(0);
				break;
			case 'a':
				stat_msg("Append.");
				edit_tag(edit_idx, 1, 0);
				break;
			case 'i':
				stat_msg("Insert.");
				edit_tag(edit_idx, 0, 0);
				break;
			case 'r':
				stat_msg("Replace.");
				edit_tag(edit_idx, 1, 1);
				break;
			case 'c':
				if (edit_idx != -1) {
					stat_msg("Cleared.");
					if (files[last_sel]->tags.strs[edit_idx][0]) {
						files[last_sel]->tags.strs[edit_idx][0] = '\0';
						if (!files[last_sel]->tags.unsaved_changes) {
							files[last_sel]->tags.unsaved_changes = files[last_sel]->need_redraw = 1;
							redraw_filelist(0);
						}
						redraw_fileinfo(edit_idx);
					}
				} else {
					stat_msg("Cannot clear filename.");
				}
				break;
			case 'e':
				if (edit_idx != -1) {
					stat_msg("Applied to all.");
					for (k = 0; k < n_files; ++k) {
						if (files[k]->selected && strcmp(files[k]->tags.strs[edit_idx],
								files[last_sel]->tags.strs[edit_idx])) {
							realloc_cp(&(files[k]->tags.strs[edit_idx]), files[last_sel]->tags.strs[edit_idx]);
							if (!files[k]->tags.unsaved_changes)
								files[k]->tags.unsaved_changes = files[k]->need_redraw = 1;
						}
					}
					redraw_filelist(0);
				} else {
					stat_msg("Cannot have repeated filename.");
				}
				break;
			case 'C':
				stat_msg("Cleared all.");
				for (k = changed = 0; k < MAX_EDITABLES; ++k) {
					if (files[last_sel]->tags.strs[k][0]) {
						files[last_sel]->tags.strs[k][0] = '\0';
						files[last_sel]->tags.unsaved_changes = changed = 1;
					}
				}
				if (changed) {
					files[last_sel]->need_redraw = 1;
					update_all(NULL);
				}
				break;
			default:
				sprintf(tmp, "Unknown key '%c' in edit mode (esc, tab, or \'!\' to leave edit mode)", k);
				stat_msg(tmp);	
			}
			if (edit_mode) /* make sure a cursor is shown for an empty entry */
				redraw_fileinfo(edit_idx);
		}
		/*
		 * Normal mode keys
		 */
		else {
			switch (k) {
			case KEY_UP: case 'k':
				clear_and_do(move_up);
				break;
			case KEY_DOWN: case 'j':
				clear_and_do(move_down);
				break;
			case ' ':
				clear_and_do(toggle_select);
				break;
			case 's':
				stat_msg("");
				if (select_or_show())
					redraw_whole_fileinfo();
				redraw_filelist(0);
				break;
			case 'g': case KEY_HOME:
				clear_and_do(goto_begin);
				break;
			case 'G': case KEY_END:
				clear_and_do(goto_end);
				break;
			case 'i': case '\t':
				if (need_sel("Select a file first.")) {
					if (last_sel != under_sel) {
						under_sel = last_sel;
						redraw_filelist(0);
					}
					stat_msg("Edit mode.");
					edit_mode = 1;
					redraw_fileinfo(edit_idx);
				}
				break;
			case 'F':
				if (need_sel("Must select files to fill.")) {
					stat_msg("Fill tags.");
					fill_selected(NULL);
					redraw_filelist(1);
					redraw_whole_fileinfo();
				}
				break;
			case 'R':
				if (need_sel("Must select files to rename.")) {
					stat_msg("Rename files.");
					rename_selected(NULL);
					redraw_filelist(1);
					redraw_whole_fileinfo();
				}
				break;
			case 'W':
				stat_msg(write_modifieds());
				redraw_filelist(0);
				break;
			case '?':
				stat_msg("Cursetag version " VERSION " (see the man page for further info)");
				break;
			default:
				sprintf(tmp, "Unknown key '%c'.", k);
				stat_msg(tmp);	
			}
		}
	}
}

int main(int argc, char* argv[])
{
	int runmode = 0;

	if (argc < 2) {
		puts("No directory given.");
		return 1;
	} else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "-v")) {
		printf("Cursetag v. %s\nPlease read the man page for instructions.\n", VERSION);
		return 0;
	} else if (argc > 3) { /* at least 3 arguments for noninteractive call */
		if (strcmp(argv[2], "-F") && strcmp(argv[2], "-R")) {
			puts("Unrecognized extra trailing arguments, quitting.");
			return 1;
		}
		runmode = 1 + (argv[2][1] == 'R');
	}

	setlocale(LC_ALL, "");
	puts("Reading directory and tags...");
	if (!get_directory(argv[1]))
		return 1;

	if (!runmode) { /* interactive mode*/
		init_curses();
		if (any_unsaved())
			stat_msg("NOTE: some automatic changes were applied!");
		mainloop();
		deinit_curses();
	} else {
		select_all();
		if (runmode == 1)
			fill_selected(argv[3]);
		else /* runmode == 2 */
			rename_selected(argv[3]);
		puts(write_modifieds());
	}
	return 0;
}

