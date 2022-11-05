/* non-editable information of an audio file */
typedef struct {
	char* filename;
	int bitrate, samplerate, channels, duration;
	unsigned long size;
} FileInfo;

enum e_Tag { T_TITLE=0, T_ARTIST, T_ALBUM, T_YEAR, T_TRACK, T_COMMENT, MAX_EDITABLES };
extern int edit_idx; /* -1: filename */

typedef struct {
	int unsaved_changes;
	char* strs[MAX_EDITABLES];
} MyTag;

void init_tag(MyTag *t);
int read_info(const char *filename, FileInfo *target, MyTag *tags);
int write_info(const char *filename, MyTag *tags);
int fix_filename(char* s);
