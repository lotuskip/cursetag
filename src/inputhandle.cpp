#include <ncurses.h>
#include <vector>
#include "io.h"
#include "filelist.h"
#include "autofill.h"
#include "encodings.h"
#include "../config.h"

using namespace std;

bool edit_mode = false;

extern int fname_print_pos; // defined in io.cpp
extern int col;

namespace
{

bool check_unsaved_changes()
{
	for(vector<FilelistEntry>::const_iterator i = files.begin(); i != files.end(); ++i)
	{
		if(i->tags.unsaved_changes)
		{
			stat_msg("There are unsaved changes! Enter '!' to confirm quit or anything else to cancel.");
			if(get_key() == '!')
				return true;
			return false;
		}
	}
	return true;
}

}


void mainloop()
{
	int k;
	for(;;)
	{
		k = get_key(); // this is just a getch()
		/*
		 * Global keys
		 */
		if(k == 'Q') // quit
		{
			if(check_unsaved_changes())	
				break;
		}
		else if(k == 'h' || k == KEY_LEFT) // scroll filelist left
		{
			if(fname_print_pos > 0)
			{
				--fname_print_pos;
				redraw_filelist(true);
			}
		}
		else if(k == 'l' || k == KEY_RIGHT) // scroll filelist right
		{
			if(fname_print_pos < longest_fname_len - col/2)
			{
				++fname_print_pos;
				redraw_filelist(true);
			}
		}
		else if(k == 'A') // select all
		{
			select_all();
			redraw_whole_fileinfo();
			redraw_filelist();
			stat_msg("All files selected.");
		}
		else if(k == 'V') // deselect all
		{
			deselect_all();
			redraw_whole_fileinfo();
			redraw_filelist();
			if(edit_mode)
			{
				edit_mode = false;
				redraw_fileinfo(idx_to_edit);
				stat_msg("Left edit mode.");
			}
			else
				stat_msg("Selection cleared.");
		}
		else if(k == '>') // move to next & select
		{
			move_down();
			if(select_or_show())
			{
				redraw_filelist();
				redraw_whole_fileinfo();
			}
		}
		else if(k == '<') // move to prev & select
		{
			move_up();
			if(select_or_show())
			{
				redraw_filelist();
				redraw_whole_fileinfo();
			}
		}
		else if(k == 'J')
		{
			if(select_down())
				redraw_whole_fileinfo();
			redraw_filelist();
		}
		else if(k == 'K')
		{
			if(select_up())
				redraw_whole_fileinfo();
			redraw_filelist();
		}
		/*
		 * Edit-mode keys
		 */
		else if(edit_mode)
		{
			switch(k)
			{
			case KEY_RESIZE: case ERR: update_reso(); break; 
			case KEY_UP: case 'k':
				if(idx_to_edit > -1)
				{
					redraw_fileinfo(idx_to_edit--);
					redraw_fileinfo(idx_to_edit);
				}
				break;
			case KEY_DOWN: case 'j':
				if(idx_to_edit < MAX_EDITABLES-1)
				{
					redraw_fileinfo(idx_to_edit++);
					redraw_fileinfo(idx_to_edit);
				}
				break;
			case 27: // escape
			case '!':
			case '\t':
				edit_mode = false;
				redraw_statics();
				stat_msg("Left edit mode.");
				break;
			case 'g': case KEY_HOME:
				goto_begin();
				if(select_or_show())
					redraw_whole_fileinfo();
				redraw_filelist();
				stat_msg("First file.");
				break;
			case 'G': case KEY_END:
				goto_end();
				if(select_or_show())
					redraw_whole_fileinfo();
				redraw_filelist();
				stat_msg("Last file.");
				break;
			case 'a': // append
				stat_msg("Append.");
				edit_tag(idx_to_edit, true);
				break;
			case 'i': // insert in beginning
				stat_msg("Insert.");
				edit_tag(idx_to_edit, false);
				break;
			case 'r': // clear and edit
				stat_msg("Replace.");
				edit_tag(idx_to_edit, true, true);
				break;
			case 'c':  // just clear
				if(idx_to_edit != -1)
				{
					if(!last_selected->tags.strs[idx_to_edit].empty())
					{
						last_selected->tags.strs[idx_to_edit].clear();
						if(!last_selected->tags.unsaved_changes)
						{
							last_selected->tags.unsaved_changes = last_selected->need_redraw = true;
							redraw_filelist();
						}
						redraw_fileinfo(idx_to_edit);
					}
					stat_msg("Cleared.");
				}
				else stat_msg("Cannot clear filename.");
				break;
			case 'e': // apply to all
				if(idx_to_edit != -1) // cannot have repeated filename!
				{
					for(vector<FilelistEntry>::iterator i = files.begin(); i != files.end(); ++i)
					{
						if(i->tags.strs[idx_to_edit] != last_selected->tags.strs[idx_to_edit])
						{
							i->tags.strs[idx_to_edit] = last_selected->tags.strs[idx_to_edit];
							if(!i->tags.unsaved_changes)
								i->tags.unsaved_changes = i->need_redraw = true;
						}
					}
					redraw_filelist();
					stat_msg("Applied to all.");
				}
				else stat_msg("Cannot have repeated filename.");
				break;
			case 'C': // clear all tags
			{
				bool changed = false;
				for(int i = 0; i < MAX_EDITABLES; ++i)
				{
					if(!last_selected->tags.strs[i].empty())
					{
						last_selected->tags.strs[i].clear();
						last_selected->tags.unsaved_changes = changed = true;
					}
				}
				if(changed)
				{
					last_selected->need_redraw = true;
					redraw_filelist();
					redraw_whole_fileinfo();
				}
				stat_msg("Cleared all.");
				break;
			}
			default:
			{
				string tmp = "Unknown key '' in edit mode (esc, tab, or \'!\' to leave edit mode)";
				ins(tmp, k, 13);
				stat_msg(tmp.c_str());	
				break;
			}
			}
		}
		/*
		 * Normal mode keys
		 */
		else // not edit_mode
		{
			switch(k)
			{
			case KEY_RESIZE: case ERR: update_reso(); break; 
			case KEY_UP: case 'k':
				move_up();
				redraw_filelist();
				break;
			case KEY_DOWN: case 'j':
				move_down();
				redraw_filelist();
				break;
			case ' ':
				toggle_select();
				redraw_whole_fileinfo();
				redraw_filelist();
				break;
			case 's':
				if(select_or_show())
					redraw_whole_fileinfo();
				redraw_filelist();
				break;
			case 'g': case KEY_HOME:
				goto_begin();
				redraw_filelist();
				break;
			case 'G': case KEY_END:
				goto_end();
				redraw_filelist();
				break;
			case 'i':
			case '\t':
				if(last_selected != files.end())
				{
					edit_mode = true;
					redraw_fileinfo(idx_to_edit);
					stat_msg("Edit mode.");
				}
				else stat_msg("Select a file first.");
				break;
			case 'F': // fill tags according to file name
				if(last_selected != files.end())
				{
					stat_msg("Fill tags.");
					fill_selected();
				}
				else stat_msg("Must select files to fill.");
				break;
			case 'R': // rename according to tags
				if(last_selected != files.end())
				{
					stat_msg("Rename files.");
					rename_selected();
				}
				else stat_msg("Must select files to rename.");
				break;
			case 'W': // write
				stat_msg(write_modifieds().c_str());
				redraw_filelist();
				break;
			case '?':
				stat_msg((string(PACKAGE_NAME) + " version " + string(PACKAGE_VERSION)
					+ " (see the man page for further info)").c_str());
				break;
			default:
			{
				string tmp = "Unknown key ''.";
				ins(tmp, k, 13);
				stat_msg(tmp.c_str());	
				break;
			}
			}
		} // not edit mode
	} // for eva
}
