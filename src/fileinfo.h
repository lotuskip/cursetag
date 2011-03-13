#ifndef FILEINFO_H
#define FILEINFO_H

#include <string>

enum e_FileType { OGG_FILE, FLAC_FILE, MP3_FILE, MP2_FILE, MP4_FILE, MPC_FILE, APE_FILE, WAVPACK_FILE, MAX_FILE_TYPE };
const std::string default_ext_for_ft[MAX_FILE_TYPE] = { ".ogg", ".flac", ".mp3", ".mp2", ".aac", ".mpc", ".ape", ".wv" };

// non-editable information of an audio file
struct FileInfo
{
	std::string filename;      // new filename
	e_FileType ft;             // see above
    int bitrate;               // Bitrate (kb/s); might be average or nominal
    int samplerate;            // Samplerate (Hz)
    int channels;              // # of channels
    int size;                  // Size of the file (bytes)
    int duration;              // Duration of the file (seconds)
};


enum e_Tag { T_TITLE=0, T_ARTIST, T_ALBUM, T_YEAR, T_TRACK, T_COMMENT, MAX_EDITABLES };

struct MyTag
{
    bool unsaved_changes;
	std::string strs[MAX_EDITABLES];
};

e_FileType filetype_by_ext(const std::string fname);
bool read_info(const char *filename, FileInfo *target, MyTag *tags);
bool write_info(const char *filename, MyTag *tags);

// given a filename (without path!) replaces all illegal characters etc. Returns whether the string was modified
bool fix_filename(std::string &s);

extern int idx_to_edit; // -1: filename, 0: title, 1: artist, ...

#endif
