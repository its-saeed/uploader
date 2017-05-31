#define BOOST_NO_CXX11_SCOPED_ENUMS
#include "UploadWorker.h"
#include <concurrentqueue.h>
#include <boost/algorithm/string.hpp>
#include <iostream>

using namespace std;

extern moodycamel::ConcurrentQueue<std::string> file_parts_queue;

UploadWorker::UploadWorker(boost::asio::io_service& io_service, size_t transmission_unit)
: io_service(io_service)
, socket_(io_service)
, timer(io_service)
, connected(false)
, transmission_unit(transmission_unit)
, file_content(new char[transmission_unit])
{
    file_stream = new std::ifstream;
	connect_socket();
}

UploadWorker::~UploadWorker()
{
	delete file_stream;
	delete file_content;
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
		connected = true;
		timer.expires_from_now(boost::posix_time::milliseconds(100));
		timer.async_wait(boost::bind(&UploadWorker::timer_timeout, this));
	}
	else
		cout << "Connection ERROR: " << error.message() << endl;
}

void UploadWorker::timer_timeout()
{
    if (!connected)
        connect_socket();
    else
    {
        std::string item;
        bool en = file_parts_queue.try_dequeue(item);
        if (en)
        {
            parse_file_parts(item);
            return init_file_part_transfer();
        }
    }
	timer.expires_from_now(boost::posix_time::milliseconds(500));
	timer.async_wait(boost::bind(&UploadWorker::timer_timeout, this));
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

void UploadWorker::init_file_part_transfer()
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

void UploadWorker::write_file_info()
{
	static constexpr char DELIMITER = '|';

	file_info.clear();
	file_info += to_string(1);		// Message Type: 1 means file part is being uploaded
	file_info += DELIMITER;
	file_info += to_string(file_part.file_info.file_id);
	file_info += DELIMITER;
	file_info += to_string(file_part.part_number);
	file_info += DELIMITER;
	file_info += to_string(file_part.part_size);
	file_info += '\n';

	boost::asio::async_write(socket_, boost::asio::buffer(file_info),
							 boost::bind(&UploadWorker::file_info_transferred, this,
										 boost::asio::placeholders::error,
										 boost::asio::placeholders::bytes_transferred));
	cout << file_part.part_number << "info transferred." << endl;
}

void UploadWorker::file_info_transferred(const boost::system::error_code& error,
		std::size_t bytes_transferred)
{
	if (error)
	{
		cout << "file transfer info ERROR: " << error.message() << endl;
		return;
	}

    start_file_transfer();
}

void UploadWorker::start_file_transfer()
{
	file_stream->read(file_content, file_part.part_size);
	socket_.async_write_some(boost::asio::buffer(file_content, file_part.part_size),
            boost::bind(&UploadWorker::some_of_file_part_transferred, this, 
                boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
}

void UploadWorker::some_of_file_part_transferred(const boost::system::error_code& error,
        std::size_t bytes_transferred)
{
	file_part.bytes_written += bytes_transferred;
	if (file_part.start_byte_index + file_part.bytes_written >= file_part.end_byte_index)
	{
		cout << file_part.part_number << " transferred." << endl;
		file_stream->close();
		timer.expires_from_now(boost::posix_time::milliseconds(100));
		timer.async_wait(boost::bind(&UploadWorker::timer_timeout, this));
		return;
	}

	buffer_start = file_content + file_part.bytes_written;
    size_t buffer_length = file_part.part_size - file_part.bytes_written;

	socket_.async_write_some(boost::asio::buffer(buffer_start, buffer_length),
            boost::bind(&UploadWorker::some_of_file_part_transferred, this, 
                boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
}
