#include "DownloadWorker.h"

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include "FileMap.h"

using namespace std;

extern FileMap file_map;

DownloadWorker::DownloadWorker(boost::asio::io_service &io_service)
: socket_(io_service)
, output_stream(new ofstream)
, download_file_part(false)
{
}

void DownloadWorker::start()
{
	boost::asio::async_read_until(socket_, buffer_,'\n',
								  boost::bind(&DownloadWorker::handle_read, shared_from_this(),
											  boost::asio::placeholders::error,
											  boost::asio::placeholders::bytes_transferred));
}

void DownloadWorker::handle_read(const boost::system::error_code& error,
								 std::size_t bytes_transferred)
{
	if (error)
	{
		socket_.close();
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
		output_stream->open("/home/saeed/download/" +
							to_string(file_part.file_info.file_id) + "_part_" + std::to_string(file_part.part_number));
		boost::asio::async_read(socket_, buffer_,
								boost::bind(&DownloadWorker::handle_read_file_content, shared_from_this(),
											boost::asio::placeholders::error,
											boost::asio::placeholders::bytes_transferred));
	}
}

void DownloadWorker::handle_read_file_content(const boost::system::error_code &error, std::size_t bytes_transferred)
{
	char buf[1024];
	std::istream stream(&buffer_);
	stream.read(buf, bytes_transferred);
	output_stream->write(buf, bytes_transferred);
	file_part.bytes_written += bytes_transferred;

	if (file_part.bytes_written >= file_part.part_size)
	{
		socket_.close();
		output_stream->close();
		file_map.file_part_downloaded(file_part.file_info.file_id);
		return;
	}
	boost::asio::async_read(socket_, buffer_,
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
		download_file_part = false;
		file_info.file_id = stol(parts.at(1));
		file_info.file_size = stol(parts.at(2));
		file_info.part_no = stol(parts.at(3));
		file_info.file_name = parts.at(4);
		file_map.insert_file(file_info);
		std::cout << "file is about to received: " <<  output;
	}
	else if (parts.at(0) == "1")		// File part
	{
		download_file_part = true;
		file_part.file_info.file_id = stol(parts.at(1));
		file_part.part_number = stol(parts.at(2));
		file_part.part_size = stol(parts.at(3));
		file_part.bytes_written = 0;
	}
}
