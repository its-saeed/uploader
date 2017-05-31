#include "FileSystemWatcher.h"
#include <string>
#include <iostream>
#include <concurrentqueue.h>
#include <boost/bind.hpp>
#include <sys/stat.h>
#include <iostream>

using namespace std;

extern moodycamel::ConcurrentQueue<std::string> file_parts_queue;

FileSystemWatcher::FileSystemWatcher(boost::asio::io_service& io_service,
        size_t transmission_unit)
: io_service(io_service)
, socket(io_service)
, timer(io_service, boost::posix_time::seconds(1))
, file_index(0)
, transmission_unit(transmission_unit)
{
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12344);
    socket.connect(endpoint);
	timer.expires_from_now(boost::posix_time::seconds(10));
    timer.async_wait(boost::bind(&FileSystemWatcher::timer_timeout, this));
    FW::WatchID watchID = file_watcher.addWatch("/home/saeed/upload/", this, true);
}

void FileSystemWatcher::timer_timeout()
{
    file_watcher.update();
	timer.expires_from_now(boost::posix_time::seconds(10));
    timer.async_wait(boost::bind(&FileSystemWatcher::timer_timeout, this));
}

void FileSystemWatcher::handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename,
        FW::Action action)
{
    if (action != FW::Actions::Add)
        return;

    struct stat statbuf;
    if (stat(std::string(dir+filename).c_str(), &statbuf) == -1)
        cout << "ERROR IN FILE SIZE" << endl;
        
    cout << "FILE size " << statbuf.st_size << endl;
    add_file_to_queue(dir, filename, statbuf.st_size);
}

void FileSystemWatcher::add_file_to_queue(const std::string path, const std::string& file_name, intmax_t file_size)
{
	std::vector<std::string> file_parts;
	int file_part_count = 0;
	for (file_part_count = 0; file_part_count < file_size / transmission_unit; ++file_part_count)
		file_parts.push_back(get_file_part_string(path + file_name, file_index, file_part_count, transmission_unit, file_part_count * transmission_unit, (file_part_count + 1) * transmission_unit));

	size_t end_of_file_size = file_size % transmission_unit;
	if (end_of_file_size)
		file_parts.push_back(get_file_part_string(path + file_name, file_index, file_part_count++, end_of_file_size, file_part_count * transmission_unit, (file_part_count * transmission_unit) + end_of_file_size));

	boost::asio::streambuf stream_buf;
	std::ostream file_info_stream(&stream_buf);
	to_be_sent.clear();
	static constexpr char DELIMITER = '|';
	to_be_sent += std::to_string(0) + DELIMITER;        // Type of message
	to_be_sent += std::to_string(file_index) + DELIMITER;        // File ID
	to_be_sent += std::to_string(file_size) + DELIMITER;     // File Size
	to_be_sent += std::to_string(file_part_count) + DELIMITER;        // File Parts
	to_be_sent += file_name + DELIMITER;                 // File name
	to_be_sent += "\n";

	cout << "TOBESENT: " << to_be_sent << endl;

	file_info_stream << to_be_sent;
	boost::asio::write(socket, boost::asio::buffer(to_be_sent));

	for(const std::string& item : file_parts)
    {
        cout << item << endl;
		file_parts_queue.enqueue(item);
    }

    ++file_index;
}

std::string FileSystemWatcher::get_file_part_string(const std::string& file_name, size_t file_id, size_t part_number, size_t part_size, size_t start_byte_index,
    size_t end_byte_index)
{
    static constexpr char DELIMITER = '|';
    std::string s = to_string(file_id) + DELIMITER + file_name + DELIMITER + to_string(part_number) + DELIMITER + to_string(part_size) + 
        DELIMITER + to_string(start_byte_index) + DELIMITER + to_string(end_byte_index);

    return s;
}
