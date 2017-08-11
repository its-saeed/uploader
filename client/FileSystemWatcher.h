#ifndef FILE_SYSTEM_WATCHER_H_
#define FILE_SYSTEM_WATCHER_H_

#include <boost/asio.hpp>
#include <iostream>
#include <boost/asio/deadline_timer.hpp>

class FileSystemWatcher
{
public:
	FileSystemWatcher(boost::asio::io_service&, size_t transmission_unit, const std::string& server_ip,
		uint16_t server_port, bool use_proxy, const std::string &proxy_ip, uint16_t proxy_port,
		const std::string& auth, const std::string &upload_dir);
    
private:
	void connect_to_server();
	void handle_connect(boost::system::error_code error);
	void extract_files_to_upload();
	void handle_read(boost::system::error_code ec, size_t bytes_received);
	void send_http_connect_message();
    std::string get_file_part_string(const std::string& file_name, size_t file_id, size_t part_number, size_t part_size, size_t start_byte_index,
            size_t end_byte_index);
    void add_file_to_queue(const std::string path, const std::string& file_name, intmax_t file_size);
    boost::asio::io_service& io_service;
    boost::asio::ip::tcp::socket socket;
    boost::asio::deadline_timer timer;
    size_t file_index;
    size_t transmission_unit;
    std::string to_be_sent;
    const std::string server_ip;
    uint16_t server_port;
    bool use_proxy;
    const std::string proxy_ip;
	uint16_t proxy_port;
	const std::string auth_token;		// proxy user:pass
	const std::string upload_path;
	char read_buffer[1024];
};

#endif
