#include <string.h>
#include <wchar.h>
#include <limits.h>
#include <stdlib.h>
#include "utils.h"

static char seq_len(const char *leadch)
{
	unsigned char lead = *((unsigned char*)leadch);
	if (lead < 0x80)
		return 1;
	if (lead >= 0xC2 && lead <= 0xDF)
		return 2;
	if (lead >= 0xE0 && lead <= 0xEF)
		return 3;
	if (lead >= 0xF0 && lead <= 0xF4)
		return 4;
	return 1; /* this is wrong and an error and will cause problems! */
}

static int invalid_cont_byte(const char *ch)
{
	unsigned char test = *((unsigned char*)ch);
	return (test < 0x80 || test > 0xBF); /* outside the allowed 2nd, 3rd or 4th byte of a multi-byte seq */
}

static char* advance(char* s, int n) /* skip 'n' symbols in 's' */
{
	for (; n > 0; --n) {
		if (!*s)
			break; /*premature end!*/
		s += seq_len(s);
	}
	return s;
}

void del(char* s, int n)
{
	s = advance(s, n);
	shift_up(s, s + seq_len(s));
}

int ins(char* s, const unsigned int c, int n)
{
	char tmp[16] = {0};
	mbstate_t mbs;
	memset(&mbs, 0, sizeof(mbs));
	size_t l = wcrtomb(tmp, c, &mbs);
	if (strlen(s) + l >= BUF_LEN)
		return 0;
	s = advance(s, n);
	shift_up(s+l, s);
	memcpy(s, tmp, l);
	return 1;
}

int num_syms(const char *s)
{
	/* assuming the string is valid UTF-8; hence simply count the non-cont bytes */
	int n = 0;
	while (*s) {
		if (invalid_cont_byte(s++))
			++n;
	}
	return n;
}

size_t mb_substr(const char* const s, int len)
{
	const char* p = s;
	for (; len && *p; --len)
		p += seq_len(p);
	return p-s;
}

void shift_up(char *beg, char *new_beg)
{
	memmove(beg, new_beg, strlen(new_beg)+1);
}

void realloc_cp(char **dest, char *src)
{
	strcpy((*dest = realloc(*dest, strlen(src)+1)), src);
}

void malloc_cp(char** dest, char *src)
{
	strcpy((*dest = malloc(strlen(src)+1)), src);
}

int min(const int a, const int b)
{
	return b + ((a-b) & ((a-b)>>(sizeof(int)*CHAR_BIT-1)));
}

int max(const int a, const int b)
{
	return a - ((a-b) & ((a-b)>>(sizeof(int)*CHAR_BIT-1)));
}
