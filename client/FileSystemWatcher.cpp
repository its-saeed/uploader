#include "FileSystemWatcher.h"
#include <string>
#include <iostream>
#include "concurrentqueue.h"
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <sys/stat.h>
#include <iostream>
#include <plog/Log.h>
#include "ProgressLogThread.h"
#include "utils.h"

using namespace std;

extern moodycamel::ConcurrentQueue<std::string> file_parts_queue;
extern moodycamel::ConcurrentQueue<ProgressLogThread::Log> logs_queue;

FileSystemWatcher::FileSystemWatcher(boost::asio::io_service& io_service, size_t transmission_unit,
									 const string &server_ip, uint16_t server_port,
									 bool use_proxy, const std::string& proxy_ip, uint16_t proxy_port,
									 const std::string& auth, const std::string& upload_dir)
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
, auth_token(auth)
, upload_path(upload_dir)
{
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
			extract_files_to_upload();
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
		server_ip + ":" + std::to_string(server_port) + " HTTP/1.1\r\n";

	if (auth_token.empty())
		connect_string += "\r\n";
	else
		connect_string += "Proxy-Authorization: Basic " + base64_encode((const unsigned char*)auth_token.data(), auth_token.size()) + "\r\n\r\n";

	boost::asio::write(socket, boost::asio::buffer(connect_string));

	boost::asio::streambuf read_buffer;
	boost::asio::read_until(socket, read_buffer, "\r\n");

	boost::asio::streambuf::const_buffers_type bufs = read_buffer.data();
	std::string str(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + read_buffer.size());

	if (str.find("200") == str.npos)
	{
		cout << "Proxy username and/or password is wrong!";
		exit(-1);
	}
	extract_files_to_upload();
}

void FileSystemWatcher::extract_files_to_upload()
{
	std::vector <string> file_names;
	boost::filesystem::path up_path(upload_path);
	if (boost::filesystem::is_regular_file(up_path))
	{
		// Open file and add all of files inside it to upload Queue
		ifstream up_stream(upload_path, ios_base::in);
		if (!up_stream.is_open())
		{
			LOG_ERROR << upload_path << " couldn't be opened." << endl;
			return;
		}

		string fileentry;

		while (std::getline(up_stream, fileentry))
			file_names.push_back(fileentry);
	}
	else if(boost::filesystem::is_directory(up_path))
	{
		// Otherwise list all files in directory and upload all of them.
		for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(up_path), {}))
			if (boost::filesystem::is_regular_file(entry))
				file_names.push_back(entry.path().string());
	}

	for (auto& entry : file_names)
	{
		boost::filesystem::path filepath(entry);
		string basename = filepath.filename().string();
		string directory = filepath.parent_path().string() + "\\";

		struct stat statbuf;
		if (stat(entry.c_str(), &statbuf) == -1)
			LOG_WARNING << "File size is not valid.";

		add_file_to_queue(directory, basename, statbuf.st_size);
	}

	// TODO: 10 should be number of concurrent uploaders.
	// set better end of files for uploaders.
	for (int i = 0; i < 10; ++i)
		file_parts_queue.enqueue("empty");
	socket.close();
}

void FileSystemWatcher::add_file_to_queue(const std::string path, const std::string& file_name, intmax_t file_size)
{
	std::vector<std::string> file_parts;
	size_t file_part_count = 0;
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
	boost::asio::streambuf read_buffer;
	boost::asio::read(socket, read_buffer,
		boost::asio::transfer_exactly(4));

	boost::asio::streambuf::const_buffers_type bufs = read_buffer.data();
	std::string str(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + read_buffer.size());

	if (stoi(str) == 200)		// 200 OK
	{
		logs_queue.enqueue({ ProgressLogThread::INIT_FILE_UPLOAD, file_name, file_part_count, 0 });
		for (const std::string& item : file_parts)
			file_parts_queue.enqueue(item);

		++file_index;
	}
}

std::string FileSystemWatcher::get_file_part_string(const std::string& file_name, size_t file_id, size_t part_number, size_t part_size, size_t start_byte_index,
    size_t end_byte_index)
{
    static constexpr char DELIMITER = '|';
    std::string s = to_string(file_id) + DELIMITER + file_name + DELIMITER + to_string(part_number) + DELIMITER + to_string(part_size) + 
        DELIMITER + to_string(start_byte_index) + DELIMITER + to_string(end_byte_index);

    return s;
}
