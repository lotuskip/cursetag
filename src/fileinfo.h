/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */
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
    int version;               // Version of bitstream (mpeg version for mp3, encoder version for ogg)
    int bitrate;               // Bitrate (kb/s)
    bool variable_bitrate;     // Is VBR?
    int samplerate;            // Samplerate (Hz)
    int mode;                  // Stereo, ... or channels for ogg
    int size;                  // Size of the file (bytes)
    int duration;              // Duration of the file (seconds)
};


enum e_Tag { T_TITLE=0, T_ARTIST, T_ALBUM, T_DISC_NUMBER, T_YEAR, T_TRACK, T_TRACK_TOTAL, T_COMMENT, MAX_EDITABLES };

struct Tag
{
    bool unsaved_changes;
	std::string strs[MAX_EDITABLES];
};

e_FileType filetype_by_ext(const std::string fname);
bool read_info(const char *filename, FileInfo *target, Tag *tags);
bool write_info(const char *filename, FileInfo *target, Tag *tags);

// given a filename (without path!) replaces all illegal characters etc. Returns whether the string was modified
bool fix_filename(std::string &s);

extern int idx_to_edit; // -1: filename, 0: title, 1: artist, ...

#endif
