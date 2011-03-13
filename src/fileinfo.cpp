#include "fileinfo.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include "encodings.h"
#include "../config.h"

#include <taglib/fileref.h>
#include <taglib/tag.h>

using namespace std;
using namespace TagLib;

int idx_to_edit = 0; 

namespace
{
const string replace_with_dash = "/\\:|";
const string replace_with_unders = "*?\"";

int get_file_size(const char *filename)
{
		struct stat statbuf;
		if(stat(filename, &statbuf))
				return -1; // error
		return statbuf.st_size;
}


bool error_ret_false(const string& msg)
{
	cerr << msg << endl;
	return false;
}


// lexical cast to string
void int_to_str(const int n, string &s)
{
	stringstream ss;
	ss << n;
	s = ss.str();
}

} // end local namespace


e_FileType filetype_by_ext(const std::string fname)
{
	// get extension and make it lowercase:
	size_t i = fname.rfind('.');
	if(i == string::npos)
		return MAX_FILE_TYPE; // not an audio file or badly named
	string ext = fname.substr(i);
	if(!ext.empty()) ext.erase(0,1); // remove '.'
	std::transform(ext.begin(), ext.end(), ext.begin(), (int(*)(int))tolower);

	// our algorithm is very brutal...
	if(ext == "ogg" || ext == "ogm")
		return OGG_FILE;
	if(ext == "flac" || ext == "fla")
		return FLAC_FILE;
	if(ext == "mp4" || ext == "m4a" || ext == "aac" || ext == "m4p")
		return MP4_FILE;
	if(ext == "mp3"
		|| ext == "mpg" || ext == "mpga") // these could be mp2, too (TODO?)
		return MP3_FILE;
	if(ext == "mp2")
		return MP2_FILE;
	if(ext == "mpc" || ext == "mp+" || ext == "mpp")
		return MPC_FILE;
	if(ext == "ape" || ext == "mac")
		return APE_FILE;
	if(ext == "wv")
		return WAVPACK_FILE;
	return MAX_FILE_TYPE;
}


bool read_info(const char *filename, FileInfo *target, MyTag *tags)
{
	if((target->ft = filetype_by_ext(filename)) == MAX_FILE_TYPE)
		return false;

	FileRef f(filename);
	if(f.isNull())
		return false;
	tags->strs[T_TITLE] = f.tag()->title().to8Bit(true);
	tags->strs[T_ARTIST] = f.tag()->artist().to8Bit(true);
	tags->strs[T_ALBUM] = f.tag()->album().to8Bit(true);
	int_to_str(f.tag()->year(), tags->strs[T_YEAR]);
	int_to_str(f.tag()->track(), tags->strs[T_TRACK]);
	tags->strs[T_COMMENT] = f.tag()->comment().to8Bit(true);

	target->bitrate = f.audioProperties()->bitrate();
	target->samplerate = f.audioProperties()->sampleRate();
	target->channels = f.audioProperties()->channels();
	target->duration = f.audioProperties()->length();

	if((target->size = get_file_size(filename)) <= 0) // can't read file anymore or it is empty??
		return false;
	
	target->filename = filename;
	target->filename = target->filename.substr(target->filename.rfind('/')+1); // get just the file, without path
	tags->unsaved_changes = fix_filename(target->filename);
	return true;
}


bool fix_filename(string &s)
{
	// TODO assuming it's UTF-8; perhaps should check and autoconvert if not?
	string refstr = s;
	size_t i;
	while((i = s.find_first_of(replace_with_dash)) != string::npos)
		s[i] = '-';
	while((i = s.find_first_of(replace_with_unders)) != string::npos)
		s[i] = '_';
	
	while((i = s.find('<')) != string::npos)
		s[i] = '(';
	while((i = s.find('>')) != string::npos)
		s[i] = ')';
	return refstr != s;
}


bool write_info(const char *filename, MyTag *tags)
{
	FileRef f(filename);
	if(f.isNull())
		return false;

	f.tag()->setTitle(tags->strs[T_TITLE]);
	f.tag()->setArtist(tags->strs[T_ARTIST]);
	f.tag()->setAlbum(tags->strs[T_ALBUM]);
	if(tags->strs[T_YEAR].empty())
		f.tag()->setYear(0); // clear
	else
		f.tag()->setYear(atoi(tags->strs[T_YEAR].c_str()));
	if(tags->strs[T_TRACK].empty())
		f.tag()->setYear(0); // clear
	else
		f.tag()->setYear(atoi(tags->strs[T_TRACK].c_str()));
	f.tag()->setComment(tags->strs[T_COMMENT]);

	return f.save();
}

