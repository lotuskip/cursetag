#include "io.h"
#include "utils.h"
#include "filelist.h"
#include <stdlib.h>
#include <string.h>

extern int row, col;

static const char wcard[MAX_EDITABLES+1] = { "tAaync" };
static const char* const tag_abbr[MAX_EDITABLES] = { "Title", "Art.", "Alb.",
	"Year", "Tr#", "Comm." };

static char buf[BUF_LEN]; /* used by tokeniser & filename constructor */
static char **tokens = NULL;
static int n_toks = 0;

static void add_token(const char* s, const int len)
{
	if (len) {
		tokens = realloc(tokens, (++n_toks)*sizeof(char*));
		strncpy((tokens[n_toks-1] = malloc(len+1)), s, len);
		tokens[n_toks-1][len] = '\0';
	}
}

static void clear_tokens(void)
{
	for (; n_toks > 0; free(tokens[--n_toks]));
	free(tokens);
	tokens = NULL;
}

static int wcard_idx(const char* s)
{
	const char* p;
	return (strlen(s) == 2 && s[0] == '%' && (p = strchr(wcard,s[1])))
		? p-wcard : -1;
}

static void tokenise(const char* const s)
{
	char* p;
	const char *ss = s;

	buf[0] = '\0';
	while (ss) {
		if ((p = strchr(ss, '%')) && *(p+1) && strchr(wcard, *(p+1))) {
			if (p != s || buf[0]) {
				strncat(buf, ss, p-ss); /* a literal before the wildcard */
				add_token(buf, strlen(buf));
				buf[0] = '\0';
				ss = p;
			}
			add_token(ss, 2); /* the wildcard */
			ss += 2;
		} else { /* end of string or not a recognized wildcard (read as literal '%') */
			if (p)
				strncpy(buf, ss, (++p)-ss);
			else
				strcpy(buf, ss);
			ss = p;
		}
	}
	if (buf[0]) /* something was left unadded */
		add_token(buf, strlen(buf));
}

static char *filename_for(FilelistEntry* i)
{
	/* replace wildcards with values and combine back into a string */
	int k  = 0, idx = 0;
	char tmp[256];
	char *p, *p2;

	for (buf[0] = '\0'; k < n_toks; ++k) {
		if ((idx = wcard_idx(tokens[k])) > -1)
			fix_filename(strcpy(tmp, i->tags.strs[idx]));
		else
			strcpy(tmp, tokens[k]);
		strcat(buf, tmp);
	}
	if ((p = strrchr(buf,'/'))) /* only rename within working dir */
		shift_up(buf, p+1);
	/* Keep existing extension if pattern doesn't provide it: */
	if ((p = strrchr(i->name,'.')) && (!(p2 = strrchr(buf,'.')) || strcmp(p,p2)))
		strcat(buf,p);
	return buf;
}

static void extract_tags_to(MyTag *target, const char* filen)
{
	size_t i;
	int k, end_k = n_toks, idx = 0; /* used here first to count '/'s */
	char *p, *p2;
	char fpath[BUF_LEN];

	strcat(strcpy(fpath, directory), filen);
	/* Remove from fpath the part not referenced in the pattern.
	 * This is done by simply counting '/'s in the pattern: */
	for (k = 0; k < n_toks; ++k)
	for (p = tokens[k]; (p = strchr(p, '/')); ++p)
		++idx;
	p = strrchr(fpath, '/');
	while (idx--) {
		while (p != fpath && *(--p) != '/');
		if (p == fpath)
			return; /* problem with matching (pattern has too many '/'s) */
	}
	shift_up(fpath, p+1);

	/* Match literal tokens in the beginning and end of pattern: */
	if (wcard_idx(tokens[(k=0)]) == -1) {
		if (strncmp(fpath, tokens[0], (i=strlen(tokens[0]))))
			return; /* problem with matching */
		shift_up(fpath, fpath+i);
		++k;
	}
	if (wcard_idx(tokens[n_toks-1]) == -1) {
		if (strcmp(fpath + (i = strlen(fpath) - strlen(tokens[n_toks-1])), tokens[n_toks-1]))
			return;
		fpath[i] = '\0';
		--end_k;
	}

	/* Match front (LHS) tokens as long as the separating literals are not
	 * spaces, or the wildcards to match are numbers: */
	for (; k < end_k; ++k) {
		idx = wcard_idx(tokens[k]);
		if (k == end_k-1) { /* this is the last token not already considered */
			realloc_cp(&(target->strs[idx]), fpath);
		} else if (!strcmp(tokens[k+1], " ") && idx != T_TRACK && idx != T_YEAR) {
			break;
		/* NOTE: we take the next token literally even if it is a wildcard!
		 * Consecutive wildcards, e.g. "%a%A" wouldn't work anyway */
		} else if (!(p = strstr(fpath, tokens[++k]))) {
			return;
		} else {
			*p = '\0';
			realloc_cp(&(target->strs[idx]), fpath);
			shift_up(fpath, (p += strlen(tokens[k])));
		}
	}

	/* Match rest from RHS regardless (just taking first matching literal).
	 * Note that this system has some faults. For instance, if the filenames
	 * are (for some obscure reason) of the form
	 *      Album 01 Foo Bar.ogg
	 * and the user uses a pattern
	 *      %a %n %t
	 * to match these, they will get title="Bar", track#="Foo" and album="Album 01". */
	while (k < end_k) {
		idx = wcard_idx(tokens[--end_k]);
		if (end_k == k) { /* last token */
			realloc_cp(&(target->strs[idx]), fpath);
		} else { /* get least occurence of last token ("strrstr()") */
			for (p = fpath; (p2 = strstr(p, tokens[end_k-1])); p = p2+1);
			if (p == fpath) /* not even one occurence */
				return;
			realloc_cp(&(target->strs[idx]), (--p) + strlen(tokens[--end_k]));
			*p = '\0';
		}
	}
}

static char* examples[] = { "", "%A/%a/%n - %t",
	"%A - %a/%n. %t", "%n. %A - %a - %t", "" };

static const char *const fill_ren[] = { "Rename;", "Fill;" };

static int get_wild_str(const int fill, const char *pattern)
{
	if (pattern) { /* non-interactive call */
		tokenise(pattern);
		return 0;
	}

	int k, pr;
	char* s;
	MyTag tags;
	WINDOW *d = newwin(5, col-2, row/2-3, 1);

	for (init_tag(&tags);;) { /* repeat until user is satisfied */
		wclear(d);
		wattrset(d, COLOR_PAIR(1));
		box(d, 0, 0);
		wmove(d, 1, 1);
		waddstr(d, fill_ren[fill]);
		waddstr(d, " enter wildcard string (up/down to scroll examples)");
		wmove(d, 3, 1);
		for (k = 0; k < MAX_EDITABLES; ++k) {
			waddstr(d, tag_abbr[k]);
			waddstr(d, ": ");
			wattrset(d, COLOR_PAIR(2));
			waddch(d, '%');
			waddch(d, wcard[k]);
			wattrset(d, COLOR_PAIR(1));
			waddch(d, ' ');
		}
		wrefresh(d);

		s = string_editor((const char**)examples, 5-!examples[4][0], d, 1, 2, 1, col-4, 1);
		if (!s[0]) /* cancelled, most likely */
			break;
		/* if entered something new, record it: */
		for (k = 1; k < 5 && strcmp(examples[k], s); ++k);
		if (k == 5)
			strcpy ((examples[4] = realloc(examples[4][0] ? examples[4] : NULL, strlen(s)+1)), s);
		clear_tokens();
		tokenise(s);

		wclear(d);
		wattrset(d, COLOR_PAIR(1));
		box(d, 0, 0);
		wmove(d, 1, 1);
		waddstr(d, "Example output: (enter to accept, esc to reformat)");
		wmove(d, 2, 1);
		if (fill) {
			for (k = 0; k < MAX_EDITABLES; ++k) { /* clear any old data */
				if (tags.strs[k])
					tags.strs[k][0] = '\0';
			}
			extract_tags_to(&tags, files[last_sel]->name);
			for (k = pr = 0; k < MAX_EDITABLES; ++k) {
				if (tags.strs[k] && tags.strs[k][0]) {
					wattrset(d, COLOR_PAIR(1));
					if (pr)
						waddstr(d, " | ");
					pr = 1;
					waddstr(d, tag_abbr[k]);
					waddstr(d, ": ");
					wattrset(d, COLOR_PAIR(4));
					waddstr(d, tags.strs[k]);
				}
			}
		} else {
			waddstr(d, filename_for(files[last_sel]));
		}
		wrefresh(d);

		do k = getch();
		while (k != 27 && k != '\n');
		if (k == '\n')
			break;
	}
	/* free up tags here since technically they introduce a recurring memory leak */
	for (k = 0; k < MAX_EDITABLES; free(tags.strs[k++]));

	wclear(d);
	wrefresh(d);
	delwin(d);
	refresh();
	return !s[0];
}

int rename_selected(const char *pattern)
{
	int k;

	if (get_wild_str(0, pattern))
		return 0;
	for (k = 0; k < n_files; ++k) {
		if (files[k]->selected && strcmp(filename_for(files[k]), files[k]->info.filename)) {
			files[k]->tags.unsaved_changes = 1;
			realloc_cp(&(files[k]->info.filename), buf);
			if (!pattern && k == last_sel)
				redraw_fileinfo(-1);
		}
	}
	return 1;
}

int fill_selected(const char *pattern)
{
	int k;

	if (get_wild_str(1, pattern))
		return 0;
	for (k = 0; k < n_files; ++k) {
		if (files[k]->selected) {
			extract_tags_to(&(files[k]->tags), files[k]->name);
			/* NOTE: we crudely assume that this actually does something
			 * even though it might be that the tags were already consistent
			 * with the filename vs. the pattern: */
			files[k]->tags.unsaved_changes = 1;
		}
	}
	return 1;
}
