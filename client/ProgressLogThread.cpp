#include "ProgressLogThread.h"
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <boost/filesystem.hpp>

#include "concurrentqueue.h"

extern moodycamel::ConcurrentQueue<ProgressLogThread::Log> logs_queue;

void ProgressLogThread::run()
{
	while (true)
	{
		Log log;
		while (bool deq = logs_queue.try_dequeue(log))
		{
			if (log.log_type == INIT_FILE_UPLOAD)
				total_file_parts[log.file_name] = log.file_parts_no;
			else if (log.log_type == PROGRESS_FILE_UPLOAD)
			{
				boost::filesystem::path p(log.file_name);
				uploaded_file_parts[p.filename().string()]++;
			}
			else if (log.log_type == END)
				return;
		}

		print_progress();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		
	}
}

void ProgressLogThread::print_progress()
{
	system("cls");

	for (auto const& imap : total_file_parts)
	{
		size_t total = imap.second;
		if (uploaded_file_parts.find(imap.first) == uploaded_file_parts.end())
			continue;

		size_t uploaded = uploaded_file_parts[imap.first];
		double percent = (double)uploaded / total * 100;

		std::cout << imap.first << "(" << percent << " %)" << std::endl;
	}
}
