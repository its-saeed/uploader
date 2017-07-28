#include "FileSystemWatcher.h"
#include <string>
#include <iostream>
#include <concurrentqueue.h>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <sys/stat.h>
#include <iostream>
#include <plog/Log.h>

using namespace std;

extern moodycamel::ConcurrentQueue<std::string> file_parts_queue;

FileSystemWatcher::FileSystemWatcher(boost::asio::io_service& io_service, size_t transmission_unit,
									 const string &server_ip, uint16_t server_port,
									 bool use_proxy, const std::string& proxy_ip, uint16_t proxy_port,
									 const std::string& upload_dir)
: io_service(io_service)
, socket(io_service)
, timer(io_service)
, file_index(0)
, transmission_unit(transmission_unit)
, server_ip(server_ip)
, server_port(server_port)
, use_proxy(use_proxy)
, proxy_ip(proxy_ip)
, proxy_port(proxy_port)
{
	FW::WatchID watchID = file_watcher.addWatch(upload_dir, this, true);
	connect_to_server();
}

void FileSystemWatcher::connect_to_server()
{
	boost::asio::ip::tcp::endpoint endpoint;
	if (use_proxy)
		endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(proxy_ip), proxy_port);
	else
		endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(server_ip), server_port);
	socket.async_connect(endpoint, boost::bind(
				&FileSystemWatcher::handle_connect, this,
				boost::asio::placeholders::error));
}

void FileSystemWatcher::handle_connect(boost::system::error_code error)
{
	if (!error)
	{
		if (!use_proxy)
		{
			timer.expires_from_now(boost::posix_time::seconds(2));
			timer.async_wait(boost::bind(&FileSystemWatcher::check_upload_dir_for_change, this));
		}
		else
			send_http_connect_message();
	}
	else
	{
		LOG_ERROR << error.message() << endl;
		timer.expires_from_now(boost::posix_time::seconds(2));
		timer.async_wait(boost::bind(&FileSystemWatcher::connect_to_server, this));
	}
}

void FileSystemWatcher::send_http_connect_message()
{
	std::string connect_string = "CONNECT " +
			server_ip + ":" + std::to_string(server_port) + " HTTP/1.1\r\n\r\n";

         boost::asio::async_read(socket,
            boost::asio::buffer(read_buffer, 1024),
            boost::bind(&FileSystemWatcher::handle_read, 
            this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
	boost::asio::write(socket, boost::asio::buffer(connect_string));

	//TODO: Check connect response, at this time I suppose it's 200OK
	timer.expires_from_now(boost::posix_time::seconds(2));
	timer.async_wait(boost::bind(&FileSystemWatcher::check_upload_dir_for_change, this));
}

void FileSystemWatcher::handle_read(boost::system::error_code ec, size_t bytes_received) {
     if (!ec)
     {

         /* do your usual handling on the incoming data */


         boost::asio::async_read(socket,
            boost::asio::buffer(read_buffer, 1024),
            boost::bind(&FileSystemWatcher::handle_read, 
            this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
     }

 }
void FileSystemWatcher::check_upload_dir_for_change()
{
    file_watcher.update();
	timer.expires_from_now(boost::posix_time::seconds(2));
	timer.async_wait(boost::bind(&FileSystemWatcher::check_upload_dir_for_change, this));
}

void FileSystemWatcher::handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename,
        FW::Action action)
{
	std::string fullname = dir + "\\" + filename;
	boost::filesystem::path up_path(fullname);
	string up_basename = up_path.filename().string();
	if (up_basename != "up.txt" || action != FW::Actions::Modified)
		return;

	ifstream up_stream(fullname, ios_base::in);

	if (!up_stream.is_open())
	{
		LOG_ERROR << "up.txt couldn't be opened." << endl;
		return;
	}

	string fileentry;

	while (std::getline(up_stream, fileentry))
	{
		boost::filesystem::path filepath(fileentry);
		string basename = filepath.filename().string();
		string directory = filepath.parent_path().string() + "\\";

		struct stat statbuf;
		if (stat(fileentry.c_str(), &statbuf) == -1)
			LOG_WARNING << "File size is not valid.";

		add_file_to_queue(directory, basename, statbuf.st_size);
	}
}

void FileSystemWatcher::add_file_to_queue(const std::string path, const std::string& file_name, intmax_t file_size)
{
	std::vector<std::string> file_parts;
	int file_part_count = 0;
	for (file_part_count = 0; file_part_count < file_size / transmission_unit; ++file_part_count)
		file_parts.push_back(get_file_part_string(path + file_name, file_index, file_part_count, transmission_unit, file_part_count * transmission_unit, (file_part_count + 1) * transmission_unit));

	size_t end_of_file_size = file_size % transmission_unit;
	if (end_of_file_size)
	{
		file_parts.push_back(get_file_part_string(path + file_name, file_index, file_part_count, end_of_file_size,
					file_part_count * transmission_unit, (file_part_count * transmission_unit) + end_of_file_size));
		++file_part_count;
	}

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

	file_info_stream << to_be_sent;
	boost::asio::write(socket, boost::asio::buffer(to_be_sent));

	for(const std::string& item : file_parts)
		file_parts_queue.enqueue(item);

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
