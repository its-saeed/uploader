#ifndef FILE_MAP_H_
#define FILE_MAP_H_

#include <unordered_map>
#include "../client/FileInfo.h"
#include <mutex>

class FileMap
{
public:
	void insert_file(const FileInfo& file_info);
	void file_part_downloaded(const FilePart &file_part, FilePartDumpBuffer file_part_buffer);
	void set_download_path(const std::string& dpath);
private:
	void write_downloaded_file_to_disk(const FileInfo& file_info);
	std::unordered_map<size_t, FileInfo> files;		/// key is file id, value is file info
	std::mutex file_map_mutex;
	std::string download_path;
};

#endif
