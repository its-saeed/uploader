#ifndef FILE_SERVER_H_
#define FILE_SERVER_H_

#include "DownloadWorker.h"
#include <boost/bind.hpp>

class FileServer
{
public:
	FileServer(boost::asio::io_service &io_service,
			size_t transmission_unit,
			uint16_t server_port);

private:
	void start_accept();
	void heartbeat_timer_expired();
	void handle_accept(DownloadWorker::pointer new_connection,
			const boost::system::error_code& error);

	boost::asio::ip::tcp::acceptor acceptor;
	size_t transmission_unit;
	boost::asio::deadline_timer heartbeat_timer;
};

#endif
