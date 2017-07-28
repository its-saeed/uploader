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
	UploadWorker(boost::asio::io_service& io_service, size_t transmission_unit,
				 const std::string& server_ip, uint16_t server_port,
				 bool use_proxy, const std::string& proxy_ip, uint16_t proxy_port);
	~UploadWorker();
    
private:
    void handle_connect(const boost::system::error_code& error);
	void send_http_connect_message();
	void file_info_transferred(const boost::system::error_code& error,
            std::size_t bytes_transferred);
	void some_of_file_part_transferred(const boost::system::error_code& error,
			std::size_t bytes_transfered);
    void parse_file_parts(const std::string& item);
	void write_file_info();
    void start_file_transfer();
	void timer_timeout();
    void connect_socket();
    void init_file_part_transfer();

    FilePart file_part;
	std::string file_info;

    boost::asio::io_service& io_service;
    boost::asio::ip::tcp::socket socket_;
    std::ifstream* file_stream;
    boost::asio::deadline_timer timer;
    bool connected;
    size_t transmission_unit;
    char* file_content;
    char* buffer_start;
    const std::string server_ip;
    uint16_t server_port;
    bool use_proxy;
    const std::string proxy_ip;
	uint16_t proxy_port;
};

#endif
