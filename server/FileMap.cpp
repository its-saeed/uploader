#include "FileMap.h"

#include <iostream>
#include <string>
#include <fstream>

using namespace std;

void FileMap::insert_file(const FileInfo &file_info)
{
	std::lock_guard<std::mutex> lock(file_map_mutex);
	files.insert({file_info.file_id, file_info});
}

void FileMap::file_part_downloaded(const FilePart& file_part, FilePartDumpBuffer file_part_buffer)
{
	size_t file_id = file_part.file_info.file_id;
	FileInfo downloaded_file;
	{
		std::lock_guard<std::mutex> lock(file_map_mutex);

		auto itr = files.find(file_id);
		if (itr == files.end())
		{
			cout << "file does not exist. file id: " << file_id << endl;
			return;
		}

		FileInfo& file_info = itr->second;
		file_info.part_no -= 1;
		file_info.file_parts.insert({file_part.part_number, file_part_buffer});

		if (file_info.part_no != 0)
			return;

		downloaded_file = std::move(itr->second);
		files.erase(itr);
	}

	write_downloaded_file_to_disk(downloaded_file);
}

void FileMap::write_downloaded_file_to_disk(const FileInfo& file_info)
{
	std::string file_name("/home/saeed/download/" + file_info.file_name);
	ofstream output_stream(file_name);

	if (!output_stream.is_open())
	{
		cout << "ERRRO opening file\n";
		return;
	}

	for (auto file_part_buffer : file_info.file_parts)
	{
		output_stream.write(file_part_buffer.second.get_buffer_raw_pointer(),
							file_part_buffer.second.part_size);
	}

	output_stream.close();
}
