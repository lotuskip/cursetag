#ifndef ENCODINGS_H
#define ENCODINGS_H

#include <string>
#include <stdexcept>

class utf_error : public std::exception {};

void del(std::string &s, int n); // remove nth symbol (not just one char) from utf-8 string
void ins(std::string &s, const unsigned int c, int n); // interpret c as utf-8 and insert into s before nth symbol

// Returns the number of utf-8 symbols (often != s.size())
int num_syms(const std::string &s);

// Returns a substring, given beginning index and length in utf-8 symbols.
// Made to return char* because we always pass the result to ncurses to print.
// 'beg' must be small enough, but if 'len' is too long, the "rest of the string"
// will be returned without segfaults.
const char* mb_substr(const std::string &s, int beg, int len);

#endif
