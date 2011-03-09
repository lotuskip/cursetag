#include "fileinfo.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <sys/stat.h>
#include "encodings.h"
#include "../config.h"

#include <taglib/fileref.h>

using namespace std;
using namespace TagLib;

int idx_to_edit = 0; 

namespace
{

int get_file_size(const char *filename)
{
		struct stat statbuf;
		if(stat(filename, &statbuf))
				return -1; // error
		return statbuf.st_size;
}

const string fopen_error = "Error opening file ";

bool error_ret_false(const string& msg)
{
	cerr << msg << endl;
	return false;
}

} // endl local namespace


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


bool read_info(const char *filename, FileRef &ref)
{
	FileRef f = new FileRef(filename);
	if((target->ft = filetype_by_ext(filename)) == MAX_FILE_TYPE)
		return false;

	if(!(read_function[target->ft](filename, target, tags)))
		return false;
	
	if((target->size = get_file_size(filename)) <= 0) // can't read file anymore or it is empty??
		return false;
	
	target->filename = filename;
	target->filename = target->filename.substr(target->filename.rfind('/')+1); // get just the file, without path
	tags->unsaved_changes = fix_filename(target->filename);
	return true;
}


bool write_info(const char *filename, const FileRef *ref)
{
	return write_function[target->ft](filename, tags);
}

