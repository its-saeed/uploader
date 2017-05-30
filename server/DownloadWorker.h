#ifndef DOWNLOAD_WORKER_H_
#define DOWNLOAD_WORKER_H

#include <boost/asio.hpp>
#include <fstream>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "../client/FileInfo.h"

class DownloadWorker : public boost::enable_shared_from_this<DownloadWorker>
{
public:
    typedef boost::shared_ptr<DownloadWorker> pointer;

    static pointer create(boost::asio::io_service& io_service, size_t transmission_unit)
    {
        return pointer(new DownloadWorker(io_service, transmission_unit));
    }

	boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

	void start();

private:
	DownloadWorker(boost::asio::io_service& io_service, size_t transmission_unit);

    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);
	void handle_read_file_content(const boost::system::error_code& error, std::size_t bytes_transferred);
	void parse_incoming_data();

	boost::asio::ip::tcp::socket socket_;
    std::string message_;
    boost::asio::streambuf buffer_;
	FilePart file_part;
	FileInfo file_info;
	bool download_file_part;
    size_t transmission_unit;
	FilePartDumpBuffer file_part_buffer;
};


#endif
