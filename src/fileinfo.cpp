#include "fileinfo.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <sys/stat.h>
#include "encodings.h"
#include "../config.h"

#ifdef ENABLE_OGG
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#endif
#ifdef ENABLE_FLAC
#include <FLAC++/all.h>
#endif
#ifdef ENABLE_MP3
#include <id3/tag.h>
#endif
#ifdef ENABLE_MP4
#include <mp4v2/mp4v2.h>
#endif


using namespace std;

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

const string fopen_error = "Error opening file ";

bool error_ret_false(const string& msg)
{
	cerr << msg << endl;
	return false;
}

// all codec-specific handling is local:

#ifdef ENABLE_OGG
string query_ogg_tag(vorbis_comment *vc, const string &key)
{
	char* rawstr = 0;
	if((rawstr = vorbis_comment_query(vc, key.c_str(), 0)))
		return validate_utf8(rawstr);
	//else
	return "";
}

const string ogg_entry_name[MAX_EDITABLES] = {
"TITLE", "ARTIST", "ALBUM", "DISCNUMBER", "DATE", "TRACKNUMBER", "TRACKTOTAL", "DESCRIPTION" };

void write_ogg_tag(vorbis_comment *vc, const e_Tag t, const string &value)
{
	vorbis_comment_add_tag(vc, ogg_entry_name[t].c_str(), value.c_str());
}
#endif

bool read_ogg_info(const char *filename, FileInfo *target, Tag *tags)
{
#ifdef ENABLE_OGG
	FILE *file;
	if(!(file = fopen(filename, "rb")))
		return error_ret_false(fopen_error + filename);

    OggVorbis_File vf;
    vorbis_info *vi;
    vorbis_comment *vc;
	int ret;
    if(!(ret = ov_open(file, &vf, 0, 0)))
    {
		/// info:
        if((vi = ov_info(&vf, -1)))
        {
            target->version = vi->version;
            target->mode = vi->channels;
            target->samplerate = vi->rate;
            target->bitrate = vi->bitrate_nominal/1000;
            target->duration = ov_time_total(&vf, -1);
        }
		else
			cerr << "Ogg Vorbis (file " << filename << "): the specified bitstream does not exist or the "
                        "file has been initialized improperly." << endl;

		/// tag:
        if((vc = ov_comment(&vf, -1)))
        {
			for(int i = 0; i < MAX_EDITABLES; ++i)
					tags->strs[i] = query_ogg_tag(vc, ogg_entry_name[i]);
        }
		else
			cerr << "Ogg Vorbis (file " << filename << "): the specified bitstream does not exist or the "
                        "file has been initialized improperly." << endl;

        ov_clear(&vf);
    }
	else
    {
        // Because not closed by ov_clear()
        fclose(file);
		cerr << "Ogg Vorbis read error, file " << filename << ", code " << ret << endl;
		// could handle the various error codes and give more precise info (TODO?)
		return false;
    }
	return true;
#else
	return false; // not supported
#endif
}

bool write_ogg_info(const char *filename, Tag *tags)
{
#ifdef ENABLE_OGG
	// TODO
	return true;
#else
	return false; // not supported
#endif
}


#ifdef ENABLE_FLAC
const string flac_entry_name[MAX_EDITABLES] = {
"TITLE", "ARTIST", "ALBUM", "DISCNUMBER", "DATE", "TRACKTOTAL", "TRACKNUMBER", "DESCRIPTION" };
#endif

bool read_flac_info(const char *filename, FileInfo *target, Tag *tags)
{
#ifdef ENABLE_FLAC
	FLAC::Metadata::StreamInfo si;
	if(!FLAC::Metadata::get_streaminfo(filename, si))
		return error_ret_false(string("Failed to read FLAC streaminfo, file ") + filename);

	// the info
	double duration = si.get_length(); // in ms!
	target->duration = (int)duration/1000;
    target->version = 0; // Not defined for FLAC
    if(duration > 0)
        target->bitrate = target->size*8/duration/1000; // TODO: Approximation; needs to remove tag size!
    target->samplerate = si.get_sample_rate();
    target->mode = si.get_channels();

	// the tags
	FLAC::Metadata::VorbisComment vc;
	if(!FLAC::Metadata::get_tags(filename, vc))
		return error_ret_false(string("Error reading tags, file ") + filename);
	FLAC::Metadata::VorbisComment::Entry entry;
	string name;
	int j;
	for(unsigned int i = 0; i < vc.get_num_comments(); ++i)
	{
		entry = vc.get_comment(i);
		name = entry.get_field_name();
		j = 0;
		while(j < MAX_EDITABLES && name != flac_entry_name[j])
			++j;
		if(j < MAX_EDITABLES && tags->strs[j].empty())
			tags->strs[j] = entry.get_field_value();
	}
    return true;
#else
	return false; // not supported
#endif
}

bool write_flac_info(const char *filename, Tag *tags)
{
#ifdef ENABLE_FLAC
	FLAC::Metadata::Chain chain;
	if(!chain.is_valid())
		return error_ret_false("Could not allocate FLAC chain!");
	if(!chain.read(filename))
		return error_ret_false(string("Error opening FLAC file \"") + filename + "\" for writing!");
	FLAC::Metadata::Iterator iter;
	if(!iter.is_valid())
		return error_ret_false("Could not allocate FLAC iterator!");
	iter.init(chain);
	bool no_vorbis_block = false;
	// we are only interested in vorbis comments:
	while(iter.get_block_type() != FLAC__METADATA_TYPE_VORBIS_COMMENT)
	{
		if(!iter.next()) // last block
		{
			no_vorbis_block = true;
			break;
		}
	}
	if(no_vorbis_block)
	{
		// TODO: the entire block needs to be added!
	}
	else // iter points to the vorbis comment; update
	{
		// To mark which tags are present in the file already:
		bool present_tags[MAX_EDITABLES] = { false, false, false, false, false,
			false, false, false };
		// it is a vorbis comment block, so this is safe:
		FLAC::Metadata::VorbisComment vc = **(iter.get_block());
		if(!vc.is_valid())
			return error_ret_false("Could not construct vorbis comment!");
		FLAC::Metadata::VorbisComment::Entry entry;
		string name;
		int j;
		unsigned int i;
		// Update entries that are present:
		for(i = 0; i < vc.get_num_comments(); ++i)
		{
			entry = vc.get_comment(i);
			if(!entry.is_valid())
				return error_ret_false("Could not extract vc entry!");
			name = entry.get_field_name();
			j = 0;
			while(j < MAX_EDITABLES && name != flac_entry_name[j])
				++j;
			if(j < MAX_EDITABLES)
			{
				present_tags[j] = true;
				if(!entry.set_field_value(tags->strs[j].c_str())
					|| !vc.set_comment(j, entry))
					return error_ret_false("Error modifying vc entry!");
			}
		}
		// Add any missing entries:
		for(i = 0; i < MAX_EDITABLES; ++i)
		{
			if(!present_tags[i] && !tags->strs[i].empty())
			{
				if(!entry.set_field_name(flac_entry_name[i].c_str())
					|| !entry.set_field_value(tags->strs[i].c_str())
					|| !vc.append_comment(entry))
					return error_ret_false("Error adding vc entry!");
			}
		}
	}
	// Done adding/updating, write:
	if(!chain.write())
		return error_ret_false("Error writing FLAC chain!");
    return true;
#else
	return false; // not supported
#endif
}


#ifdef ENABLE_MP3
const ID3_FrameID mp3_entry_name[MAX_EDITABLES] = {
ID3FID_TITLE, ID3FID_LEADARTIST, ID3FID_ALBUM, ID3FID_PARTINSET, ID3FID_YEAR, ID3FID_TRACKNUM,
ID3FID_TRACKNUM, /*intentionally again; total tracks is included in tracknum */ ID3FID_COMMENT };
#endif

bool read_mpeg_info(const char *filename, FileInfo *target, Tag *tags)
{
#ifdef ENABLE_MP3
    ID3_Tag id3_tag;
	id3_tag.Link(filename);

	/// Header stuff
    const Mp3_Headerinfo* headerInfo;
    if((headerInfo = id3_tag.GetMp3HeaderInfo()))
    {
        switch(headerInfo->version)
        {
        case MPEGVERSION_1:
            target->version = 1;
            break;
        case MPEGVERSION_2:
            target->version = 2;
            break;
        case MPEGVERSION_2_5:
			target->version = 2;
            break;
        default: break;
        }

        target->samplerate = headerInfo->frequency;

        switch(headerInfo->modeext)
        {
        case MP3CHANNELMODE_STEREO:
            target->mode = 0;
            break;
        case MP3CHANNELMODE_JOINT_STEREO:
            target->mode = 1;
            break;
        case MP3CHANNELMODE_DUAL_CHANNEL:
            target->mode = 2;
            break;
        case MP3CHANNELMODE_SINGLE_CHANNEL:
            target->mode = 3;
            break;
        default: break;
        }

        if(headerInfo->vbr_bitrate <= 0)
        {
            target->variable_bitrate = false;
            target->bitrate = headerInfo->bitrate/1000;
        }
		else
        {
            target->variable_bitrate = true;
            target->bitrate = headerInfo->vbr_bitrate/1000;
        }

        target->duration = headerInfo->time;
    }

	/// Tag stuff
	ID3_Frame* frame;
	for(int i = 0; i < T_TRACK; ++i)
	{
		if((frame = id3_tag.Find(mp3_entry_name[i])))
			tags->strs[i] = validate_utf8(frame->GetField(ID3FN_TEXT)->GetRawText());	
	}
	if((frame = id3_tag.Find(ID3FID_TRACKNUM)))
	{
			tags->strs[T_TRACK] = validate_utf8(frame->GetField(ID3FN_TEXT)->GetRawText());
			size_t i = tags->strs[T_TRACK].find('/');
			if(i != string::npos)
			{
				tags->strs[T_TRACK_TOTAL] = tags->strs[T_TRACK].substr(i+1);
				tags->strs[T_TRACK].erase(i);
			}
	}
	if((frame = id3_tag.Find(mp3_entry_name[T_COMMENT])))
		tags->strs[T_COMMENT] = validate_utf8(frame->GetField(ID3FN_TEXT)->GetRawText());	

    id3_tag.Clear();
	return true;
#else
	return false; // not supported
#endif
}

bool write_mpeg_info(const char *filename, Tag *tags)
{
#ifdef ENABLE_MP3
    ID3_Tag id3_tag(filename);
	
	ID3_Frame* frame;
	for(int i = 0; i < T_TRACK; ++i)
	{
		if(!(frame = id3_tag.Find(mp3_entry_name[i]))) // if missing, create
			id3_tag.AttachFrame((frame = new ID3_Frame(mp3_entry_name[i])));
		frame->GetField(ID3FN_TEXT)->Set(tags->strs[i].c_str());	
	}
	if(!(frame = id3_tag.Find(ID3FID_TRACKNUM)))
		id3_tag.AttachFrame((frame = new ID3_Frame(ID3FID_TRACKNUM)));
	string tmpstr = tags->strs[T_TRACK];
	if(!tags->strs[T_TRACK_TOTAL].empty())
		tmpstr += '/' + tags->strs[T_TRACK_TOTAL];
	frame->GetField(ID3FN_TEXT)->Set(tmpstr.c_str());
	if(!(frame = id3_tag.Find(mp3_entry_name[T_COMMENT])))
		id3_tag.AttachFrame((frame = new ID3_Frame(mp3_entry_name[T_COMMENT])));
	frame->GetField(ID3FN_TEXT)->Set(tags->strs[T_COMMENT].c_str());

	id3_tag.Update(); // write changes to file
	return true;
#else
	return false; // not supported
#endif
}


bool read_mpc_info(const char *filename, FileInfo *target, Tag *tags)
{
#ifdef ENABLE_MPC
	return false; //TODO
#else
	return false; // not supported
#endif
}

bool write_mpc_info(const char *filename, Tag *tags)
{
#ifdef ENABLE_MPC
	return false; //TODO
#else
	return false; // not supported
#endif
}


bool read_ape_info(const char *filename, FileInfo *target, Tag *tags)
{
#ifdef ENABLE_APE
	return false; //TODO
#else
	return false; // not supported
#endif
}

bool write_ape_info(const char *filename, Tag *tags)
{
#ifdef ENABLE_APE
	return false; //TODO
#else
	return false; // not supported
#endif
}


bool read_wavpack_info(const char *filename, FileInfo *target, Tag *tags)
{
#ifdef ENABLE_WAVPACK
	return false; //TODO
#else
	return false; // not supported
#endif
}

bool write_wavpack_info(const char *filename, Tag *tags)
{
#ifdef ENABLE_WAVPACK
	return false; //TODO
#else
	return false; // not supported
#endif
}


bool read_aac_info(const char *filename, FileInfo *target, Tag *tags)
{
#ifdef ENABLE_MP4
	return false; //TODO
#else
	return false; // not supported
#endif
}

bool write_aac_info(const char *filename, Tag *tags)
{
#ifdef ENABLE_MP4
	return false; //TODO
#else
	return false; // not supported
#endif
}


bool (*read_function[MAX_FILE_TYPE])(const char *filename, FileInfo *target, Tag *tags) =
{ read_ogg_info, read_flac_info, read_mpeg_info, read_mpeg_info, read_aac_info,
	read_mpc_info, read_ape_info, read_wavpack_info };

bool (*write_function[MAX_FILE_TYPE])(const char *filename, Tag *tags) =
{ write_ogg_info, write_flac_info, write_mpeg_info, write_mpeg_info, write_aac_info,
	write_mpc_info, write_ape_info, write_wavpack_info };

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


bool read_info(const char *filename, FileInfo *target, Tag *tags)
{
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


bool write_info(const char *filename, FileInfo *target, Tag *tags)
{
	return write_function[target->ft](filename, tags);
}

