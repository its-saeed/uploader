#include "DownloadWorker.h"

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include "FileMap.h"

using namespace std;

extern FileMap file_map;

DownloadWorker::DownloadWorker(boost::asio::io_service &io_service,
        size_t transmission_unit)
: socket_(io_service)
, download_file_part(false)
, transmission_unit(transmission_unit)
{
}

void DownloadWorker::start()
{
	boost::asio::read_until(socket_, buffer_,'\n');
	parse_incoming_data();
	if (!download_file_part)
		boost::asio::async_read_until(socket_, buffer_, '\n',
									  boost::bind(&DownloadWorker::handle_read, shared_from_this(),
												  boost::asio::placeholders::error,
												  boost::asio::placeholders::bytes_transferred));
	else
	{
		file_part_buffer.part_size = file_part.part_size;
		socket_.async_read_some(boost::asio::buffer(file_part_buffer.get_buffer_raw_pointer(), file_part.part_size),
								boost::bind(&DownloadWorker::handle_read_file_content, shared_from_this(),
											boost::asio::placeholders::error,
											boost::asio::placeholders::bytes_transferred));
	}
}

void DownloadWorker::handle_read(const boost::system::error_code& error,
								 std::size_t bytes_transferred)
{
	if (error)
	{
        cout << "Read ERROR" << error.message() << endl;
		return;
	}

	if (bytes_transferred)
		parse_incoming_data();

	if (!download_file_part)
		boost::asio::async_read_until(socket_, buffer_, '\n',
									  boost::bind(&DownloadWorker::handle_read, shared_from_this(),
												  boost::asio::placeholders::error,
												  boost::asio::placeholders::bytes_transferred));
	else
	{
		file_part_buffer.part_size = file_part.part_size;
		socket_.async_read_some(boost::asio::buffer(file_part_buffer.get_buffer_raw_pointer(), file_part.part_size),
								boost::bind(&DownloadWorker::handle_read_file_content, shared_from_this(),
											boost::asio::placeholders::error,
											boost::asio::placeholders::bytes_transferred));
	}
}

void DownloadWorker::handle_read_file_content(const boost::system::error_code &error, std::size_t bytes_transferred)
{
    if (error)
        cout << "Content Read Error " << error.message() << endl;

    if (!bytes_transferred)
        return;

	file_part.bytes_written += bytes_transferred;
    size_t remaining_bytes = file_part.part_size - file_part.bytes_written;

	if (remaining_bytes == 0)
	{
        download_file_part = false;
		file_map.file_part_downloaded(file_part, FilePartDumpBuffer(file_part_buffer));
		cout << "part downloaded: file id: " << file_part.file_info.file_id << ", part no: " << file_part.part_number << endl;
		start();
		return;
	}

	socket_.async_read_some(boost::asio::buffer(file_part_buffer.get_buffer_raw_pointer() + file_part.bytes_written, remaining_bytes),
            boost::bind(&DownloadWorker::handle_read_file_content, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
}

void DownloadWorker::parse_incoming_data()
{
	std::string output;
	std::istream response_stream(&buffer_);
	getline(response_stream, output, '\n');

	std::vector<std::string> parts;
	split(parts, output, boost::is_any_of("|"));

	if (parts.at(0) == "0")		// File Info
	{
		if (parts.size() < 5)
			cout << "error in signaling data: " << output << endl;
		download_file_part = false;
		file_info.file_id = stol(parts.at(1));
		file_info.file_size = stol(parts.at(2));
		file_info.part_no = stol(parts.at(3));
		file_info.file_name = parts.at(4);
		file_map.insert_file(file_info);
		cout << "file info: " << file_info.file_id << endl;
	}
	else if (parts.at(0) == "1")		// File part
	{
		if (parts.size() < 4)
			cout << "error in signaling data: " << output << endl;
		download_file_part = true;
		file_part.file_info.file_id = stol(parts.at(1));
		file_part.part_number = stol(parts.at(2));
		file_part.part_size = stol(parts.at(3));
		file_part.bytes_written = 0;
		cout << "part info, file id: "<< file_part.file_info.file_id << ", part no:" << file_part.part_number << endl;
	}
}
