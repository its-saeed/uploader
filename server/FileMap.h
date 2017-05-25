#ifndef FILE_MAP_H_
#define FILE_MAP_H_

#include <unordered_map>
#include "../client/FileInfo.h"
#include <mutex>

class FileMap
{
public:
	void insert_file(const FileInfo& file_info);
	void file_part_downloaded(size_t file_id);
private:
	std::unordered_map<size_t, FileInfo> files;		/// key is file id, value is file info
	std::mutex file_map_mutex;
};

#endif
