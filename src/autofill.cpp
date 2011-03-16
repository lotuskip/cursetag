#include "autofill.h"
#include "io.h"
#include "filelist.h"
#include "encodings.h"
#include <algorithm>

using namespace std;

extern int row, col;

namespace
{

const char wcard[MAX_EDITABLES+1] = {
	't', // T_TITLE
	'A', // T_ARTIST
	'a', // T_ALBUM
	'y', // T_YEAR
	'n', // T_TRACK
	'c', // T_COMMENT
	'\0' // make this a valid C string
};

const string tag_abbr[MAX_EDITABLES] = { "Title: ", "Art.: ", "Alb.: ",
	"Year: ", "Tr#: ", "Comm.: " };

void tokenise(string s, vector<string> &res)
{
	size_t i = 0;
	string tmp = "";
	while(!s.empty())
	{
		i = s.find('%');
		if(i != string::npos && i < s.size()-1 && string(wcard).find(s[i+1]) != string::npos)
		{
			if(i > 0 || !tmp.empty())
			{
				res.push_back(s.substr(0, i) + tmp); // the literal before the wildcard
				s.erase(0, i);
				tmp.erase();
			}
			res.push_back(s.substr(0, 2)); // the wildcard
			s.erase(0, 2);
		}
		else // not a wildcard, so take it as a literal '%'
		{
			if(i != string::npos)
				++i;
			tmp += s.substr(0, i);
			s.erase(0, i);
		}
	}
	if(!tmp.empty()) // if something was left unadded
		res.push_back(tmp);
}


// NOTE: tokens can't be passed by reference! cf. the call in rename_selected()
string filename_for(vector<FilelistEntry>::iterator i, vector<string> tokens)
{
	// replace wildcards with values and combine back into a string
	int idx = 0;
	string result = "";
	for(vector<string>::iterator it = tokens.begin(); it != tokens.end(); ++it)
	{
		// if token is a wildcard:
		if(it->size() == 2 && (*it)[0] == '%' && (idx = string(wcard).find((*it)[1])) != string::npos)
		{
			*it = i->tags.strs[idx];
			fix_filename(*it); // it might contain illegal characters; must replace them here
		}
		result += *it;
	}
	// We only rename *files*, so anything before the last '/' is unacceptable:
	if((idx = result.rfind('/')) != string::npos)
		result.erase(0, idx+1);
	// Keep existing extension:
	if((idx = i->name.rfind('.')) != string::npos)
		result += i->name.substr(idx);

	return result;
}


void extract_tags_to(MyTag *target, vector<string> tokens, string filepath)
{
	// The last token can include the extension or not:
	size_t i = filepath.rfind('.');
	// NOTE: may assume i != string::npos since this is the name of a once-accepted file!
	string s = filepath.substr(i);
	filepath.erase(i);
	if((i = tokens.back().rfind(s)) != string::npos)
	{
		if(i + s.size() < tokens.back().size()) // there is something *after* the extension; that won't do
			return; // filename does not match pattern
		// else the extension is there; get rid of it
		tokens.back().erase(i);
		if(i == 0) // removed it completely!
			tokens.pop_back();
	}

	/* After that, we simply match literals to filename (returning if impossible)
	 * and extract the needed tags in between. Note that this algorithm doesn't
	 * check the possibility that the same wildcard appears more than once --- the
	 * value is simply overwritten. */
	int idx = 0;
	while(!tokens.empty())
	{
		s = tokens.back(); // save some typing
		if(s.size() == 2 && s[0] == '%' && (idx = string(wcard).find(s[1])) != string::npos) // is wildcard
		{
			tokens.pop_back();
			if(tokens.empty()) // that was the last token!
			{
				i = filepath.rfind('/');
				if(i != string::npos)
					++i;
				filepath.erase(0, i);
				target->strs[idx] = filepath;
			}
			else // there is a next token
			{
				/* NOTE: we take the next token literally even if it is a wildcard!
				 * Consecutive wildcards, e.g. "%a%A" wouldn't work anyway */
				if((i = filepath.rfind(tokens.back())) == string::npos)
					return; // problem with matching
				target->strs[idx] = filepath.substr(i + tokens.back().size());
				filepath.erase(i);
				tokens.pop_back();
			}
		}
		else // not wildcard; match literal
		{
			if((i = filepath.rfind(tokens.back())) == string::npos)
				return; // problem with matching
			filepath.erase(i);
			tokens.pop_back();
		}
	}
}


vector<string> examples;

string get_wild_str(const bool fill)
{
	// init examples if not already done
	if(examples.empty())
	{
		examples.push_back("");
		examples.push_back("%A/%a/%n - %t");
		examples.push_back("%A - %a/%n. %t");
		examples.push_back("%n. %A - %a - %t");
	}

	WINDOW *dialog = newwin(5, col-2, row/2-3, 1);

	string s = "";
    vector<string> tokens;
	int k;
	for(;;) // repeat until user is satisfied
	{
		wclear(dialog);
		wattrset(dialog, COLOR_PAIR(0));
		box(dialog, 0, 0);
		wmove(dialog, 1, 1);
		if(fill)
			waddstr(dialog, "Fill;");
		else
			waddstr(dialog, "Rename;");
		waddstr(dialog, " enter wildcard string (up/down to scroll examples)");
		wmove(dialog, 3, 1);
		for(k = 0; k < MAX_EDITABLES; ++k)
		{
			waddstr(dialog, tag_abbr[k].c_str());
			wattrset(dialog, COLOR_PAIR(1));
			waddch(dialog, '%');
			waddch(dialog, wcard[k]);
			wattrset(dialog, COLOR_PAIR(0));
			waddch(dialog, ' ');
		}
		wrefresh(dialog);

		s = string_editor(examples, dialog, 1, 2, true, col-4, true);
		if(s.empty()) // cancelled, most likely
			break;
		//else if entered something new, record it:
		if(find(examples.begin(), examples.end(), s) == examples.end())
			examples.push_back(s);

		wclear(dialog);
		wattrset(dialog, COLOR_PAIR(0));
		box(dialog, 0, 0);
		wmove(dialog, 1, 1);
		waddstr(dialog, "Example output: (enter to accept, esc to reformat)");
		wmove(dialog, 2, 1);
		// print the example
		tokens.clear();
    	tokenise(s, tokens);
		if(fill)
		{
			MyTag tags;
			extract_tags_to(&tags, tokens, directory + last_selected->name);
			for(k= 0; k < MAX_EDITABLES; ++k)
			{
				if(!tags.strs[k].empty())
				{
					waddstr(dialog, tag_abbr[k].c_str());
					wattrset(dialog, COLOR_PAIR(3));
					waddstr(dialog, tags.strs[k].c_str());
					wattrset(dialog, COLOR_PAIR(0));
					waddstr(dialog, " | ");
				}
			}
		}
		else
            waddstr(dialog, filename_for(last_selected, tokens).c_str());
		wrefresh(dialog);

		do k = getch();
		while(k != 27 && k != '\n');
		if(k == '\n')
			break;
		// else start over
	}

	wclear(dialog);
	wrefresh(dialog);
	delwin(dialog);
	refresh();
	return s;
}

}

void rename_selected()
{
	string ws = get_wild_str(false);
	if(ws.empty())
		return; // cancelled
    vector<string> tokens;
    tokenise(ws, tokens);
	string tmp;
	for(vector<FilelistEntry>::iterator it = files.begin(); it != files.end(); ++it)
	{
		if(it->selected)
		{
			tmp = filename_for(it, tokens);
			if(tmp != it->info.filename)
			{
				it->tags.unsaved_changes = true;
				it->info.filename = tmp;
				if(it == last_selected)
					redraw_fileinfo(-1);
			}
		}
	}
	redraw_filelist(true);
}


void fill_selected()
{
	string ws = get_wild_str(true);
	if(ws.empty())
		return; // cancelled
    vector<string> tokens;
    tokenise(ws, tokens);
	for(vector<FilelistEntry>::iterator it = files.begin(); it != files.end(); ++it)
	{
		if(it->selected)
		{
			extract_tags_to(&(it->tags), tokens, directory + it->name);
			// NOTE: we crudely assume that this actually does something:
			it->tags.unsaved_changes = true;
		}
	}
	redraw_filelist(true);
	redraw_whole_fileinfo();
}
