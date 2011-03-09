#ifndef FILEINFO_H
#define FILEINFO_H

#include <string>

namespace TagLib { class FileRef; }

enum e_FileType { OGG_FILE, FLAC_FILE, MP3_FILE, MP2_FILE, MP4_FILE, MPC_FILE, APE_FILE, WAVPACK_FILE, MAX_FILE_TYPE };
const std::string default_ext_for_ft[MAX_FILE_TYPE] = { ".ogg", ".flac", ".mp3", ".mp2", ".aac", ".mpc", ".ape", ".wv" };

e_FileType filetype_by_ext(const std::string fname);
TagLib::FileRef* read_info(const char *filename);
bool write_info(const char *filename, const TagLib::FileRef *ref);

// given a filename (without path!) replaces all illegal characters etc. Returns whether the string was modified
bool fix_filename(std::string &s);

extern int idx_to_edit; // -1: filename, 0: title, 1: artist, ...

#endif
