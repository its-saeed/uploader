#ifndef UPLOAD_WORKER_H_
#define UPLOAD_WORKER_H_

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <fstream>

class UploadWorker
{
public:
    UploadWorker(boost::asio::io_service& io_service);
    
private:
    void handle_connect(const boost::system::error_code& error);
    void handle_write(const boost::system::error_code& error,
            std::size_t bytes_transfered);
    void parse_file_parts(const std::string& item);
    void write_file_part();

    boost::asio::io_service& io_service;
    boost::asio::ip::tcp::socket socket_;
    std::string current_file;
    size_t send_start_index;
    size_t send_end_index;
    size_t send_bytes_written;
    std::ifstream* file_stream;
};

#endif
