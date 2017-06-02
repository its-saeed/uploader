#include "DownloadWorker.h"

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <thread>
#include <plog/Log.h>
#include "FileMap.h"

using namespace std;

extern FileMap file_map;

DownloadWorker::DownloadWorker(boost::asio::io_service &io_service,
		size_t transmission_unit)
: socket_(io_service)
, download_file_part(false)
, transmission_unit(transmission_unit)
, temp_buffer(new char[transmission_unit])
, write_to_buffer_index(0)
, consume_index(0)
{
	std::thread th{[&io_service](){io_service.run();}};
	th.detach();
}

void DownloadWorker::start()
{
	size_t buffer_size = transmission_unit - write_to_buffer_index;

    if (download_file_part)
        buffer_size = std::min(file_part.part_size - file_part.bytes_written, buffer_size);

	socket_.async_read_some(boost::asio::buffer(temp_buffer.get() + write_to_buffer_index, buffer_size),
								  boost::bind(&DownloadWorker::handle_read, shared_from_this(),
											  boost::asio::placeholders::error,
											  boost::asio::placeholders::bytes_transferred));
}

void DownloadWorker::handle_read(const boost::system::error_code& error,
								 std::size_t bytes_transferred)
{
	write_to_buffer_index += bytes_transferred;
	if (error)
	{
		LOG_ERROR << error.message();
		return;
	}

	if (download_file_part)
		return file_part_bytes_received();

	// Otherwise we need to see if we have sufficient bytes to get file info section.
	string buffer_string(temp_buffer.get() + consume_index, write_to_buffer_index - consume_index);
	if (buffer_string.find('\n') != string::npos)
		parse_signaling_bytes(buffer_string.substr(0, buffer_string.find('\n')));

	start();
}

void DownloadWorker::parse_signaling_bytes(const std::string& signaling)
{
	std::vector<std::string> parts;
	split(parts, signaling, boost::is_any_of("|"));

	if (parts.at(0) == "0")		// File Info
	{
		download_file_part = false;
		file_info.file_id = stol(parts.at(1));
		file_info.file_size = stol(parts.at(2));
		file_info.part_no = stol(parts.at(3));
		file_info.file_name = parts.at(4);
		file_map.insert_file(file_info);
		write_to_buffer_index = 0;
		consume_index = 0;
	}
	else if (parts.at(0) == "1")		// File part
	{
		download_file_part = true;
		file_part.file_info.file_id = stol(parts.at(1));
		file_part.part_number = stol(parts.at(2));
		file_part.part_size = stol(parts.at(3));
		file_part.bytes_written = 0;
		file_part_buffer.part_size = file_part.part_size;
		consume_index += signaling.size() + 1; 		// +1 for \n
	}
}

void DownloadWorker::file_part_bytes_received()
{
	char* dest = file_part_buffer.get_buffer_raw_pointer() + file_part.bytes_written;
	const char* src = temp_buffer.get() + consume_index;
	size_t size = write_to_buffer_index - consume_index;
	size_t remaining_bytes = file_part.part_size - file_part.bytes_written;
	memcpy(dest, src, std::min(size, remaining_bytes));
	file_part.bytes_written += std::min(size, remaining_bytes);
	consume_index += std::min(size, remaining_bytes);

	if (size == std::min(size, remaining_bytes))
	{
		consume_index = 0;
		write_to_buffer_index = 0;
	}

	check_if_part_downloaded();
}

void DownloadWorker::check_if_part_downloaded()
{
	int64_t remaining_bytes = file_part.part_size - file_part.bytes_written;

	if (remaining_bytes == 0)
	{
		download_file_part = false;
		file_map.file_part_downloaded(file_part, FilePartDumpBuffer(file_part_buffer));
	}

	assert(remaining_bytes >= 0);

	start();
}
