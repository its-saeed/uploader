#define BOOST_NO_CXX11_SCOPED_ENUMS
#include "UploadWorker.h"
#include <concurrentqueue.h>
#include <boost/algorithm/string.hpp>
#include <iostream>

using namespace std;

extern moodycamel::ConcurrentQueue<std::string> file_parts_queue;

UploadWorker::UploadWorker(boost::asio::io_service& io_service)
: io_service(io_service)
, socket_(io_service)
, timer(io_service)
{
    file_stream = new std::ifstream;
	timer.expires_from_now(boost::posix_time::seconds(1));
	timer.async_wait(boost::bind(&UploadWorker::timer_timeout, this));
}

void UploadWorker::timer_timeout()
{
	std::string item;
	bool en = file_parts_queue.try_dequeue(item);
	if (en)
	{
		parse_file_parts(item);
		connect_socket();
	}
	else
	{
		timer.expires_from_now(boost::posix_time::seconds(1));
		timer.async_wait(boost::bind(&UploadWorker::timer_timeout, this));
	}
}

void UploadWorker::parse_file_parts(const std::string& item)
{
	std::vector<std::string> parts;
	split(parts, item, boost::is_any_of("|"));
	file_part.file_info.file_id = stol(parts.at(0));
	file_part.file_info.file_name = parts.at(1);
	file_part.part_number = stol(parts.at(2));
	file_part.part_size = stol(parts.at(3));
	file_part.start_byte_index = stol(parts.at(4));
	file_part.end_byte_index = stol(parts.at(5));
	file_part.bytes_written = 0;
}

void UploadWorker::connect_socket()
{
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12345);
	socket_.async_connect(endpoint, boost::bind(
				&UploadWorker::handle_connect, this,
				boost::asio::placeholders::error));
}

void UploadWorker::handle_connect(const boost::system::error_code& error)
{
    if (!error)
    {
		file_stream->open(file_part.file_info.file_name);
		if (file_stream->is_open())
		{
			file_stream->seekg(file_part.start_byte_index);
			write_file_info();
		}
		else
			cout << "ERROR: can't open file";
    }
	else
		cout << "ERROR: " << error.message();
}

void UploadWorker::write_file_info()
{
	static constexpr char DELIMITER = '|';
	std::string file_info;

	file_info += to_string(1);		// Message Type: 1 means file part is being uploaded
	file_info += DELIMITER;
	file_info += to_string(file_part.file_info.file_id);
	file_info += DELIMITER;
	file_info += to_string(file_part.part_number);
	file_info += DELIMITER;
	file_info += to_string(file_part.part_size);
	file_info += '\n';

	boost::asio::async_write(socket_, boost::asio::buffer(file_info),
							 boost::bind(&UploadWorker::handle_write, this,
										 boost::asio::placeholders::error,
										 boost::asio::placeholders::bytes_transferred, false));
}

void UploadWorker::handle_write(const boost::system::error_code& error,
		std::size_t bytes_transferred, bool file_content_transferred)
{
	if (error)
	{
		cout << "ERROR: " << error.message();
		return;
	}
	if (file_content_transferred)
		file_part.bytes_written += bytes_transferred;
    write_file_part();
}

void UploadWorker::write_file_part()
{
	if (file_part.start_byte_index + file_part.bytes_written >= file_part.end_byte_index)
	{
		socket_.close();
		file_stream->close();
		timer.expires_from_now(boost::posix_time::seconds(1));
		timer.async_wait(boost::bind(&UploadWorker::timer_timeout, this));
		return;
	}

	char file_content[1024] = {'m'};
	file_stream->read(file_content, file_part.part_size);
	cout << "part size: " <<file_part.part_size << " Bytes read: " << file_stream->gcount() << endl;

	cout << std::string(file_content, file_part.part_size);
	boost::asio::async_write(socket_, boost::asio::buffer(file_content, file_part.part_size),
            boost::bind(&UploadWorker::handle_write, this, 
                boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred, true));
}
