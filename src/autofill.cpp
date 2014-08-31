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


// returns wildcard index or -1 if isn't
int token_is_wildcard(const string &s)
{
	int idx;
	if(s.size() == 2 && s[0] == '%' && (idx = string(wcard).find(s[1])) != string::npos)
		return idx;
	return -1;
}

void extract_tags_to(MyTag *target, vector<string> tokens, string filepath)
{
	/* Remove from filepath the path that is not referenced in the pattern.
	 * This is done by simply counting '/'s in the pattern: */
	size_t i;
	int idx = 0; // (used here first to count '/'s)
	vector<string>::iterator tokenit = tokens.begin();
	while(tokenit != tokens.end())
	{
		i = tokenit->find('/'); // same token might contain several:
		while(i != string::npos)
		{
			++idx;
			i = (*tokenit).find('/', i+1);
		}
		++tokenit;
	}
	i = filepath.rfind('/', string::npos);
	while(idx--)
	{
		if(!i)
			return; // problem with matching (pattern has too many '/'s)
		i = filepath.rfind('/', i-1);
	}
	filepath.erase(0, i+1); // erase unreferenced part of path

	// Match literal tokens in the beginning and end of pattern:
	if(token_is_wildcard(tokens.front()) == -1)
	{
		i = tokens.front().size();
		if(filepath.substr(0, i) != tokens.front())
			return; // problem with matching
		filepath.erase(0, i);
		tokens.erase(tokens.begin());
	}
	if(token_is_wildcard(tokens.back()) == -1)
	{
		// back is a literal token
		i = filepath.size() - tokens.back().size();
		if(i > filepath.size() || filepath.substr(i) != tokens.back())
			return; // problem with matching
		filepath.erase(i);
		tokens.pop_back();
	}

	/* Match front (LHS) tokens as long as the separating literals are not
	 * spaces, or the wildcards to match are numbers: */
	while(!tokens.empty())
	{
		idx = token_is_wildcard(tokens.front());
		if(tokens.size() == 1) // that was the last token!
		{
			target->strs[idx] = filepath;
			tokens.clear();
		}
		else if(tokens[1] == " " && idx != T_TRACK && idx != T_YEAR)
			break; // don't consider this
		else
		{
			/* NOTE: we take the next token literally even if it is a wildcard!
			 * Consecutive wildcards, e.g. "%a%A" wouldn't work anyway */
			if((i = filepath.find(tokens[1])) == string::npos)
				return; // problem with matching
			target->strs[idx] = filepath.substr(0, i);
			filepath.erase(0, i + tokens[1].size());
			tokens.erase(tokens.erase(tokens.begin()));
		}
	}

	/* Match rest from RHS regardless (just taking first matching literal).
	 * Note that this system has some faults. For instance, if the filenames
	 * are (for some obscure reason) of the form
	 *      Album 01 Foo Bar.ogg
	 * and the user uses a pattern
	 *      %a %n %t
	 * to match these, they will find title="Bar", track#="Foo" and
	 * album="Album 01", which is wrong. */
	while(!tokens.empty())
	{
		idx = token_is_wildcard(tokens.back());
		tokens.pop_back();
		if(tokens.empty()) // that was the last token!
		{
			i = filepath.rfind('/');
			if(i != string::npos)
				++i;
			filepath.erase(0, i);
			target->strs[idx] = filepath;
		}
		else
		{
			if((i = filepath.find(tokens.back())) == string::npos)
				return; // problem with matching
			target->strs[idx] = filepath.substr(i + tokens.back().size());
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
		wattrset(dialog, COLOR_PAIR(1));
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
			wattrset(dialog, COLOR_PAIR(2));
			waddch(dialog, '%');
			waddch(dialog, wcard[k]);
			wattrset(dialog, COLOR_PAIR(1));
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
		wattrset(dialog, COLOR_PAIR(1));
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
					wattrset(dialog, COLOR_PAIR(4));
					waddstr(dialog, tags.strs[k].c_str());
					wattrset(dialog, COLOR_PAIR(1));
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

} // end local namespace

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
			/* NOTE: we crudely assume that this actually does something
			 * even though it might be that the tags were already consistent
			 * with the filename vs. the pattern: */
			it->tags.unsaved_changes = true;
		}
	}
	redraw_filelist(true);
	redraw_whole_fileinfo();
}
