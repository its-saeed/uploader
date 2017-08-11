#pragma once

#include <map>
#include <string>

class ProgressLogThread
{
public:
	enum LogType {
		INIT_FILE_UPLOAD,
		PROGRESS_FILE_UPLOAD,
		END
	};

	struct Log
	{
		LogType log_type;
		std::string file_name;
		size_t file_parts_no;
		size_t current_uploaded_file_part;
	};

	void run();

private:
	void print_progress();
	std::map<std::string, size_t> total_file_parts;		// Key is file name, value parts
	std::map<std::string, size_t> uploaded_file_parts;		// Key is file name, value uploaded
};