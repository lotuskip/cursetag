#ifndef ENCODINGS_H
#define ENCODINGS_H

#include <string>
#include <stdexcept>

class utf_error : public std::exception {};

std::string validate_utf8(const char* rawstr);

void del(std::string &s, int n); // remove nth symbol (not just one char) from utf-8 string
void ins(std::string &s, const unsigned int c, int n); // interpret c as utf-8 and insert into s before nth symbol

// Returns the number of utf-8 symbols (often != s.size())
int num_syms(const std::string &s);

#endif
