#ifndef UPLOAD_WORKER_H_
#define UPLOAD_WORKER_H_

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <fstream>
#include <boost/asio/deadline_timer.hpp>

#include "FileInfo.h"

class UploadWorker
{
public:
    UploadWorker(boost::asio::io_service& io_service);
    
private:
    void handle_connect(const boost::system::error_code& error);
	void handle_write(const boost::system::error_code& error,
			std::size_t bytes_transfered, bool file_content_transferred);
    void parse_file_parts(const std::string& item);
	void write_file_info();
	void write_file_part();
	void timer_timeout();
    void connect_socket();

    FilePart file_part;

    boost::asio::io_service& io_service;
    boost::asio::ip::tcp::socket socket_;
    std::ifstream* file_stream;
    boost::asio::deadline_timer timer;
};

#endif
