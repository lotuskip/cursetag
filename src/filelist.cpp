#include "filelist.h"
#include "encodings.h"
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <iostream>
#include <cerrno>
#include <cstdio>
#include <cmath>

using namespace std;

namespace
{

void check_empty_selection()
{
	for(vector<FilelistEntry>::iterator i = files.begin(); i != files.end(); ++i)
	{
		if(i->selected)
		{
			last_selected = i;
			return;
		}
	}
	// no files selected
	last_selected = files.end();
}

void fix_track_tags()
{
	// Because our tag reading reads the tag numbers as integers which are then
	// converted into strings, there is no proper padding with zeros. We do that
	// here:
	short maxtrack = 0, tmp;
	vector<FilelistEntry>::iterator i;
	for(i = files.begin(); i != files.end(); ++i)
	{
		if((tmp = atoi(i->tags.strs[T_TRACK].c_str())) > maxtrack)
			maxtrack = tmp;
	}
	if((tmp = log10(float(maxtrack))) > 0)
	{
		++tmp; // this is how many symbols we want
		for(i = files.begin(); i != files.end(); ++i)
			i->tags.strs[T_TRACK].insert(0, max(0,int(tmp-i->tags.strs[T_TRACK].size())), '0');
	}
}

} // end local namespace

bool operator<(const FilelistEntry &a, const FilelistEntry &b)
{
		return a.name < b.name;
}

string directory;
vector<FilelistEntry> files;
vector<FilelistEntry>::iterator under_selector;
vector<FilelistEntry>::iterator last_selected;
int longest_fname_len = 0;

bool get_directory(const char* name)
{
	struct stat st_file_info;
	if(stat(name, &st_file_info))
	{
		cerr << "No such directory: " << name << endl;
		return false;
	}//else...

	if(!(S_ISDIR(st_file_info.st_mode)))
	{
		cerr << '\'' << name << "' is not a directory." << endl;
		return false;
	}

	// now check if we have any audio in there and populate the list
	DIR *dp;
	if((dp = opendir(name)) == NULL)
	{
		cerr << "Error opening directory " << name << endl;
	    return false;
	}// else...

	directory = name;
	// make sure the final '/' is there:
	if(directory[directory.size()-1] != '/')
		directory += '/';
	longest_fname_len = num_syms(directory);

    struct dirent *dirp;
	FileInfo tmp_finfo;
	MyTag tmp_tag;
	int tmp_i;
	while((dirp = readdir(dp)) != NULL)
	{
		// Attempt to read file info; if this fails, the file is not of a supported format
		// or cannot be read.
		if(read_info((directory+string(dirp->d_name)).c_str(), &tmp_finfo, &tmp_tag))
		{
			files.push_back(FilelistEntry(string(dirp->d_name), tmp_finfo, tmp_tag));
			// Get the longest file name length (in utf-8 symbols):
			if((tmp_i = num_syms(files.back().name)) > longest_fname_len)
				longest_fname_len = tmp_i;
		}
	}
	closedir(dp);
	
	if(!files.size())
	{
		cerr << "Did not find any recognised audio files in " << name << endl;
		return false;
	}

	fix_track_tags();

	sort(files.begin(), files.end());
	under_selector = files.begin();
	last_selected = files.end();
	return true;
}


void toggle_select()
{
	if((under_selector->selected = !(under_selector->selected)))
	{
		last_selected = under_selector;
		// move_down(); // Ranger-like selection
	}
	else
		check_empty_selection();
	under_selector->need_redraw = true;
}


bool select_or_show()
{
	if(!under_selector->selected)
	{
		under_selector->need_redraw = under_selector->selected = true;
		// move_down(); // Ranger-like selection
	}
	if(under_selector != last_selected)
	{
		last_selected = under_selector;
		return true;
	}
	return false;
}


void deselect_all()
{
	for(vector<FilelistEntry>::iterator i = files.begin(); i != files.end(); ++i)
	{
		if(i->selected)
		{
			i->selected = false;
			i->need_redraw = true;
		}
	}
	last_selected = files.end();
}

void select_all()
{
	for(vector<FilelistEntry>::iterator i = files.begin(); i != files.end(); ++i)
	{
		if(!i->selected)
			i->selected = i->need_redraw = true;
	}
	under_selector = last_selected = files.end()-1;
}

bool select_down()
{
	vector<FilelistEntry>::iterator i = files.begin();
	for(; i != under_selector; ++i)
	{
		if(i->selected)
		{
			i->selected = false;
			i->need_redraw = true;
		}
	}
	// 'i' points to under_selector now
	bool retval = !i->selected;

	for(; i != files.end(); ++i)
	{
		if(!(i->selected))
			i->selected = i->need_redraw = true;
	}

	// Note: we don't change the selection any more; we used to:
#if 0
	if(last_selected != files.end()-1)
	{
		last_selected = files.end()-1;
		return true;
	}
	return false
#endif
	return retval;
}

bool select_up()
{
	bool retval = !under_selector->selected;

	vector<FilelistEntry>::iterator i = files.begin();
	for(; i != under_selector+1; ++i)
	{
			if(!(i->selected))
				i->selected = i->need_redraw = true;
	}
	for(; i != files.end(); ++i)
	{
			if(i->selected)
			{
				i->selected = false;
				i->need_redraw = true;
			}
	}
#if 0
	if(last_selected != files.begin())
	{
			last_selected = files.begin();
			return true;
	}
	return false;
#endif
	return retval;
}

void goto_begin()
{
	if(under_selector != files.begin())
	{
		under_selector->need_redraw = true;
		(under_selector = files.begin())->need_redraw = true;
	}
}

void goto_end()
{
	if(under_selector != files.end()-1)
	{
		under_selector->need_redraw = true;
		(under_selector = files.end()-1)->need_redraw = true;
	}
}

void move_up()
{
	if(under_selector != files.begin())
	{
		under_selector->need_redraw = true;
		--under_selector;
		under_selector->need_redraw = true;
	}
}

void move_down()
{
	if(under_selector != files.end()-1)
	{
		under_selector->need_redraw = true;
		++under_selector;
		under_selector->need_redraw = true;
	}
}

void invert_selection()
{
	for(vector<FilelistEntry>::iterator i = files.begin(); i != files.end(); ++i)
	{
		i->selected = !(i->selected);
		i->need_redraw = true;
	}
	check_empty_selection();
}


string write_modifieds()
{
	bool wrote_sumn = false;
	for(vector<FilelistEntry>::iterator i = files.begin(); i != files.end(); ++i)
	{
		if(i->selected && i->tags.unsaved_changes)
		{
			if(!write_info((directory + i->name).c_str(), &(i->tags)))
				return "Error writing tag for file \'"  + i->name + "\'!";
			// else tag was written
			if(i->name != i->info.filename) // needs to be renamed
			{
				// i->name is the old name, i->info.filename the new
				if(rename((directory + i->name).c_str(),
					(directory + i->info.filename).c_str()))
				{
					string s = "Could not rename file \'" + i->name + "\' into \'"
						+ i->info.filename + "\' ";
					switch(errno)
					{
					case EACCES: case EPERM: return s + "(check permissions)";
					case EROFS: return s + "(read-only)";
					case ENAMETOOLONG: return s + "(name is too long)";
					case EEXIST: case ENOTEMPTY: case EINVAL: case EISDIR:
						return s + "(other name problem)";
					}
					return s + "(other problem)";
				}
				i->name = i->info.filename;
			}
			// if here, all went ok
			wrote_sumn = true;
			i->need_redraw = true;
			i->tags.unsaved_changes = false;
		}
	}
	if(wrote_sumn)
		return "Tags for selected files succesfully written.";
	//else
	return "No files with unsaved changes selected!";
}
