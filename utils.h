void del(char* s, int n);
int ins(char* s, const unsigned int c, int n);
int num_syms(const char* s);

/* Returns number of bytes to read from 's' to get 'len' printable symbols.
 * 'len' may be too long, and then basically strlen(s) is returned. */
size_t mb_substr(const char* s, int len);

/* These are just string handling shorthands, nothing to do with Unicode. */
void shift_up(char *beg, char *new_beg);
void realloc_cp(char **dest, char *src);
void malloc_cp(char **dest, char *src);

int min(int a, int b);
int max(int a, int b);

#define BUF_LEN 512 /* ins() above will not let s grow beyond this */
