#include "FileServer.h"

#include <iostream>
#include <plog/Log.h>

using namespace std;

FileServer::FileServer( boost::asio::io_service &io_service,
		size_t transmission_unit,
		uint16_t server_port)
: acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), server_port))
, transmission_unit(transmission_unit)
, heartbeat_timer(io_service)
{
	heartbeat_timer.expires_from_now(boost::posix_time::seconds(4));
	heartbeat_timer.async_wait(boost::bind(&FileServer::heartbeat_timer_expired, this));
	start_accept();
}

void FileServer::start_accept()
{
	DownloadWorker::pointer new_connection =
			DownloadWorker::create(acceptor.get_io_service(), transmission_unit);

	acceptor.async_accept(new_connection->socket(),
						  boost::bind(&FileServer::handle_accept, this, new_connection,
									  boost::asio::placeholders::error));
}

void FileServer::heartbeat_timer_expired()
{
	heartbeat_timer.expires_from_now(boost::posix_time::seconds(4));
	heartbeat_timer.async_wait(boost::bind(&FileServer::heartbeat_timer_expired, this));
}

void FileServer::handle_accept(DownloadWorker::pointer new_connection, const boost::system::error_code &error)
{
	if (!error)
	{
		cout << "connection stablished" << endl;
		new_connection->start();
	}
	else
		LOG_ERROR << "Error connection: " << error.message();

	start_accept();
}
