#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <taglib/tag_c.h>
#include "fileinfo.h"
#include "utils.h"

int edit_idx = 0;

static unsigned long get_file_size(const char *filename)
{
	struct stat statbuf;
	return stat(filename, &statbuf) ? 0 : statbuf.st_size;
}

void init_tag(MyTag *t)
{
	int k;
	for (k = t->unsaved_changes = 0; k < MAX_EDITABLES; ++k)
		t->strs[k] = NULL;
}

static void read_into(MyTag *tags, const int idx, char *s)
{
	strcpy((tags->strs[idx] = malloc(min(BUF_LEN, strlen(s)+1))), s);
}

static void read_int(MyTag *tags, const int idx, unsigned int i)
{
	char tmp[16];
	sprintf(tmp, "%i", i);
	malloc_cp(&(tags->strs[idx]), tmp);
}

int read_info(const char *filename, FileInfo *target, MyTag *tags)
{
	char buf[BUF_LEN];
	TagLib_File* f = taglib_file_new(filename);
	if (!f || !taglib_file_is_valid(f))
		return 0;

	TagLib_Tag* t = taglib_file_tag(f);
	const TagLib_AudioProperties* ap = taglib_file_audioproperties(f);

	read_into(tags, T_TITLE, taglib_tag_title(t));
	read_into(tags, T_ARTIST, taglib_tag_artist(t));
	read_into(tags, T_ALBUM, taglib_tag_album(t));
	read_int(tags, T_YEAR, taglib_tag_year(t));
	read_int(tags, T_TRACK, taglib_tag_track(t));
	read_into(tags, T_COMMENT, taglib_tag_comment(t));

	target->bitrate = taglib_audioproperties_bitrate(ap);
	target->samplerate = taglib_audioproperties_samplerate(ap);
	target->channels = taglib_audioproperties_channels(ap);
	target->duration = taglib_audioproperties_length(ap);

	if (!(target->size = get_file_size(filename)))
		return 0; /* can't read file anymore or it is empty?? */
	
	malloc_cp(&(target->filename), strcpy(buf, strrchr(filename, '/')+1));
	tags->unsaved_changes = fix_filename(target->filename);
	return 1;
}

int fix_filename(char* s)
{
	char refstr[BUF_LEN];
	char *p;

	strcpy(refstr, s);
	for (p = s; (p = strchr(p, '/')); *p = '-');
	for (p = strrchr(s, '.'); p && *p; ++p) /* make extension lowerc. */
		*p = tolower(*p);
	return strcmp(refstr,s);
}


int write_info(const char *filename, MyTag *tags)
{
	TagLib_File* f = taglib_file_new(filename);
	if (!f)
		return 0;

	TagLib_Tag* t = taglib_file_tag(f);
	taglib_id3v2_set_default_text_encoding(TagLib_ID3v2_UTF8);
	taglib_tag_set_title(t, tags->strs[T_TITLE]);
	taglib_tag_set_artist(t, tags->strs[T_ARTIST]);
	taglib_tag_set_album(t, tags->strs[T_ALBUM]);
	taglib_tag_set_year(t, strtol(tags->strs[T_YEAR], NULL, 10));
	taglib_tag_set_track(t, strtol(tags->strs[T_TRACK], NULL, 10));
	taglib_tag_set_comment(t, tags->strs[T_COMMENT]);
	return taglib_file_save(f);
}
