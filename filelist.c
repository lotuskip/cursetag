#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "utils.h"
#include "filelist.h"
#include "io.h"

static void check_empty_selection(void)
{
	int k;
	for (k = 0; k < n_files && !files[k]->selected; ++k);
	last_sel = k;
}

static void fix_track_tags(void)
{
	/* Pad track numbers with zeros so that if largest track# is 14, track 1 => "01".
	 * This is important so that files renamed by tags are properly sorted. */
	int m = 0, tmp, k;
	for (k = 0; k < n_files; ++k) {
		if ((tmp = strtol(files[k]->tags.strs[T_TRACK], NULL, 10)) > m)
			m = tmp;
	}
	for (tmp = 1, k = m; (k /= 10) > 0; ++tmp); /* Avoid using log10 & math.h dep. */
	if (tmp > 1) { /* this is how many symbols we want */
		for (k = 0; k < n_files; ++k) {
			if ((m = strlen(files[k]->tags.strs[T_TRACK])) < tmp) {
				files[k]->tags.strs[T_TRACK] = realloc(files[k]->tags.strs[T_TRACK], tmp+1);
				ins(files[k]->tags.strs[T_TRACK], '0', 0);
			}
		}
	}
}

static int cmp(const void* a, const void* b)
{
	return strcmp((*(FilelistEntry**)a)->name, (*(FilelistEntry**)b)->name);
}

char directory[PATH_MAX];
char buf[BUF_LEN];
FilelistEntry** files = NULL;
int n_files = 0;
int under_sel;
int last_sel;
int longest_fname_len = 0;

int get_directory(const char* name)
{
	int i;
	struct stat st_file_info;
	DIR *dp;
	struct dirent *dirp;
	FileInfo tmp_finfo;
	MyTag tmp_tag;

	if (!realpath(name, directory)) {
		printf("Failed to resolve path '%s'", name);
		return 0;
	} else if (stat(directory, &st_file_info)) {
		fprintf(stderr, "No such directory: %s\n", directory);
		return 0;
	} else if (!(S_ISDIR(st_file_info.st_mode))) {
		fprintf(stderr, "'%s' is not a directory.\n", directory);
		return 0;
	} else if ((dp = opendir(directory)) == NULL) {
		fprintf(stderr, "Error opening directory %s\n", directory);
		return 0;
	}
	longest_fname_len = num_syms(directory)+1;
	strcat(directory, "/");

	init_tag(&tmp_tag);
	while ((dirp = readdir(dp)) != NULL) {
		strcpy(strrchr(directory,'/')+1, dirp->d_name);
		if (read_info(directory, &tmp_finfo, &tmp_tag)) {
			files = realloc(files, (++n_files)*sizeof(FilelistEntry*));
			(files[n_files-1] = malloc(sizeof(FilelistEntry)))->selected = 0;
			files[n_files-1]->need_redraw = 1;
			malloc_cp(&(files[n_files-1]->name), dirp->d_name);
			files[n_files-1]->info = tmp_finfo;
			files[n_files-1]->tags = tmp_tag;
			if ((i = num_syms(dirp->d_name)) > longest_fname_len)
				longest_fname_len = i;
		}
	}
	closedir(dp);
	*(strrchr(directory,'/')+1) = '\0';
	
	if (!n_files) {
		fprintf(stderr, "Did not find any recognised audio files in %s\n", name);
		return 0;
	}
	fix_track_tags();
	qsort(files, (last_sel = n_files), sizeof(FilelistEntry*), cmp);
	under_sel = 0;
	return 1;
}

void toggle_select(void)
{
	if ((files[under_sel]->selected = !(files[under_sel]->selected)))
		last_sel = under_sel;
	else
		check_empty_selection();
	files[under_sel]->need_redraw = 1;
	redraw_whole_fileinfo();
}

int select_or_show(void)
{
	if (!files[under_sel]->selected)
		files[under_sel]->need_redraw = files[under_sel]->selected = 1;
	if (under_sel != last_sel) {
		last_sel = under_sel;
		return 1;
	}
	return 0;
}

static void desel1(FilelistEntry *f)
{
	if (f->selected) {
		f->selected = 0;
		f->need_redraw = 1;
	}
}

static void sel1(FilelistEntry *f)
{
	if (!f->selected)
		f->selected = f->need_redraw = 1;
}

void deselect_all(void)
{
	int k = 0;
	while (k < n_files)
		desel1(files[k++]);
	last_sel = n_files;
}

void select_all(void)
{
	int k = 0;
	while (k < n_files)
		sel1(files[k++]);
	under_sel = last_sel = n_files-1;
}

void select_down(void)
{
	int k;
	for (k = 0; k < under_sel; ++k)
		desel1(files[k]);
	int retval = !(files[k]->selected);

	while (k < n_files)
		sel1(files[k++]);
	if (retval)
		redraw_whole_fileinfo();
}

void select_up(void)
{
	int k, retval = !files[under_sel]->selected;

	for (k = 0; k < under_sel+1; ++k)
		sel1(files[k]);
	for (; k < n_files; ++k)
		desel1(files[k]);
	if (retval)
		redraw_whole_fileinfo();
}

static void goto_n(const int n)
{
	files[under_sel]->need_redraw = 1;
	files[(under_sel = n)]->need_redraw = 1;
}

void goto_begin() { if (under_sel) goto_n(0); }
void goto_end(void) { if (under_sel < n_files-1) goto_n(n_files-1); }
void move_up(void) { if (under_sel) goto_n(under_sel-1); }
void move_down(void) { if (under_sel < n_files-1) goto_n(under_sel+1); }

void invert_selection(void)
{
	int k;
	for (k = 0; k < n_files; ++k) {
		files[k]->selected = !files[k]->selected;
		files[k]->need_redraw = 1;
	}
	check_empty_selection();
}

char *write_modifieds(void)
{
	int k, n, wrote_sumn = 0;
	struct stat st_b;
	char fpath[BUF_LEN];

	strcpy(fpath, directory);
	for (k = 0; k < n_files; ++k) {
		if (files[k]->selected && files[k]->tags.unsaved_changes) {
			strcpy(strrchr(fpath,'/')+1, files[k]->name);
			if (!write_info(fpath, &(files[k]->tags))) {
				sprintf(buf, "Error writing tag for file '%s'!", files[k]->name);
				return buf;
			}
			if (strcmp(files[k]->name, files[k]->info.filename)) {
				/* i->name is the old name, i->info.filename the new */
				strcpy(buf, fpath);
				strcpy(strrchr(buf,'/')+1, files[k]->info.filename);
				if (!stat(buf, &st_b)) {
					sprintf(buf, "Will not overwrite existing file '%s'!", files[k]->info.filename);
					return buf;
				} else if (rename(fpath, buf)) {
					sprintf(buf, "Could not rename file '%s' into '%s' (", files[k]->name,
						files[k]->info.filename);
					switch (errno) {
					case EACCES: case EPERM: strcat(buf, "check permissions)"); return buf;
					case EROFS: strcat(buf, "read-only)"); return buf;
					case ENAMETOOLONG: strcat(buf, "name is too long)"); return buf;
					case EEXIST: case ENOTEMPTY: case EINVAL: case EISDIR:
						strcat(buf, "other problem in name)"); return buf;
					}
					strcat(buf, "other problem)"); return buf;
				}
				realloc_cp(&(files[k]->name), files[k]->info.filename);
				if (longest_fname_len < (n = num_syms(files[k]->name)))
					longest_fname_len = n;
			}
			wrote_sumn = files[k]->need_redraw = 1;
			files[k]->tags.unsaved_changes = 0;
		}
	}
	return wrote_sumn ? "Selected files succesfully written."
		: "No files with unsaved changes selected!";
}
