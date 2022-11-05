#include "fileinfo.h"

typedef struct {
	char* name;
	int selected;
	int need_redraw;
	FileInfo info;
	MyTag tags;
} FilelistEntry;

extern char directory[];
extern FilelistEntry** files;
extern int n_files;
extern int under_sel;
extern int last_sel;
extern int longest_fname_len;

int get_directory(const char* name);
void toggle_select(void);
int select_or_show(void);
void deselect_all(void);
void select_all(void);
void select_down(void);
void select_up(void);
void goto_begin(void);
void goto_end(void);
void move_up(void);
void move_down(void);
void invert_selection(void);
char* write_modifieds(void);
