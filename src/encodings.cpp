#include "encodings.h"
#include <iconv.h>
#include <iostream>
#include <cerrno>
#include <string.h>

using namespace std;

namespace
{

// function attempting conversion from a given encoding
bool try_convert(const char* rawstr, char* output, size_t len, const char* enc)
{
	iconv_t ict = iconv_open("UTF-8", enc);
	if(ict == iconv_t(-1))
	{
		cerr << "Conversion from " << enc << " to wchar_t not supported!" << endl;
		return false;
	}
	bool good = (iconv(ict, const_cast<char**>(&rawstr), &len, &output, &len) != size_t(-1));
	iconv_close(ict);
	return good;
}


char seq_len(char leadch) // determine the length of the sequence starting with leadch
{
	unsigned char lead = static_cast<unsigned char>(leadch);
	if(lead < 0x80)
		return 1;
	if(lead >= 0xC2 && lead <= 0xDF)
		return 2;
	if(lead >= 0xE0 && lead <= 0xEF)
		return 3;
	if(lead >= 0xF0 && lead <= 0xF4)
		return 4;
	throw utf_error(); // invalid lead!
}


bool invalid_cont_byte(const char ch)
{
	unsigned char test = static_cast<unsigned char>(ch);
	return (test < 0x80 || test > 0xBF); // outside the only allowed 2nd, 3rd or 4th byte of a multi-byte seq
}


// move iterator forward by a complete symbol
void advance(string::iterator &i, string &s)
{
	char l = seq_len(*i);
	do { ++i; --l; }
	while(l > 0 // sequence is ready,
		&& i != s.end() // premature end of string,
		&& !invalid_cont_byte(*i)); // non-first byte invalid for continuation

	if(l > 0) // if was broken for some other reason than this, something is wrong
		throw utf_error();
}


bool valid_utf8(string &s)
{
	string::iterator i = s.begin();
	try {
	while(i != s.end())
		advance(i, s);
	}
	catch(utf_error e)
	{
		return false;
	}
    return true;
}

}


string validate_utf8(const char* rawstr)
{
	// It might be, and usually is, that the string IS valid UTF-8, in which case we do nothing to it:
	string s = string(rawstr);
	if(valid_utf8(s))
			return s;

	// Else we try to convert it
	int len = strlen(rawstr);
	char* output = new char[len];
	// try ISO-8859-1? (TODO: try others, such as system default, if not UTF-8?)
	if(!try_convert(rawstr, output, len, "ISO-8859-1"))
	{
		if(errno == EINVAL) // no luck!
		{
			// TODO: put to output an escaped string instead?
			delete[] output; return "";
		}
		else { delete[] output; return ""; }
	}
	// all good?
	s = string(output);
	delete[] output;
	return s;
}


void del(string &s, int n)
{
	string::iterator i = s.begin();
	for(; n > 0; --n)
		advance(i, s);
	char l = seq_len(*i);
	for(; l > 0; --l)
		i = s.erase(i); // NOTE: unsafe if string is improper UTF-8...
}


void ins(string &s, const unsigned int c, int n)
{
	// First interpret "n symbols" in terms of a string iterator:
	string::iterator i = s.begin();
	for(; n > 0; --n)
		advance(i, s);
	// NOTE: assuming int is 32-bit..!
	if(c < 0x80) // single byte
		s.insert(i, static_cast<unsigned char>(c));
	else if(c < 0x800) // 2 bytes
	{
		i = s.insert(i, static_cast<unsigned char>((c >> 6) | 0xc0));
		s.insert(++i, static_cast<unsigned char>((c & 0x3f) | 0x80));
	}
	else if(c < 0x10000) // 3
	{
		i = s.insert(i, static_cast<unsigned char>((c >> 12) | 0xe0));
		i = s.insert(++i, static_cast<unsigned char>(((c >> 6) & 0x3f) | 0x80));
		s.insert(++i, static_cast<unsigned char>((c & 0x3f) | 0x80));
	}
	else // 4
	{
		i = s.insert(i, static_cast<unsigned char>((c >> 18) | 0xf0));
		i = s.insert(++i, static_cast<unsigned char>(((c >> 12) & 0x3f) | 0x80));
		i = s.insert(++i, static_cast<unsigned char>(((c >> 6) & 0x3f) | 0x80));
		s.insert(++i, static_cast<unsigned char>((c & 0x3f) | 0x80));
	}
}


int num_syms(const string &s)
{
	// we assume the string is valid UTF-8, and can hence simply count the non-continuation bytes
	int n = 0;
	for(string::const_iterator i = s.begin(); i != s.end(); ++i)
	{
		if(static_cast<unsigned char>(*i) < 0x80 || static_cast<unsigned char>(*i) > 0xBF)
			++n;
	}
	return n;
}


const char* mb_substr(const string &s, int beg, int len)
{
	int b = 0;
	for(; beg > 0; --beg)
		b += seq_len(s[b]);
	int e = 0;
	for(; len > 0; --len)
	{
		if(b+e >= s.size())
			break;
		e += seq_len(s[b+e]);
	}
	return s.substr(b, e).c_str();
}

