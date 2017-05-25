#include "FileMap.h"

#include<iostream>
#include <string>

using namespace std;

void FileMap::insert_file(const FileInfo &file_info)
{
	std::lock_guard<std::mutex> lock(file_map_mutex);
	files.insert({file_info.file_id, file_info});
}

void FileMap::file_part_downloaded(size_t file_id)
{
	std::lock_guard<std::mutex> lock(file_map_mutex);

	if (files.find(file_id) == files.end())
		return;

	FileInfo& file_info = files[file_id];
	file_info.part_no -= 1;

	if (file_info.part_no == 0)
	{
		std::cout << file_info.file_name << " downloaded" << endl;

		system(std::string("cat /home/saeed/download/" + to_string(file_info.file_id) + "_* > /home/saeed/download/" + file_info.file_name).c_str());
		system(std::string("rm /home/saeed/download/" + to_string(file_info.file_id) + "_*").c_str());
	}
}
