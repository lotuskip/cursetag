#ifndef FILEINFO_H
#define FILEINFO_H

#include <string>

// non-editable information of an audio file
struct FileInfo
{
	std::string filename;      // new filename
    int bitrate;               // Bitrate (kb/s); might be average or nominal
    int samplerate;            // Samplerate (Hz)
    int channels;              // # of channels
    unsigned long size;        // Size of the file (bytes)
    int duration;              // Duration of the file (seconds)
};


enum e_Tag { T_TITLE=0, T_ARTIST, T_ALBUM, T_YEAR, T_TRACK, T_COMMENT, MAX_EDITABLES };

struct MyTag
{
    bool unsaved_changes;
	std::string strs[MAX_EDITABLES];
};

bool read_info(const char *filename, FileInfo *target, MyTag *tags);
bool write_info(const char *filename, MyTag *tags);

// given a filename (without path!) replaces all illegal characters etc. Returns whether the string was modified
bool fix_filename(std::string &s);

extern int idx_to_edit; // -1: filename, 0: title, 1: artist, ...

#endif
