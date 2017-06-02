#include "FileServer.h"

#include <iostream>

using namespace std;

FileServer::FileServer( boost::asio::io_service &io_service,
		size_t transmission_unit,
		uint16_t server_port)
: acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), server_port))
, transmission_unit(transmission_unit)
{
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

void FileServer::handle_accept(DownloadWorker::pointer new_connection, const boost::system::error_code &error)
{
	if (!error)
		new_connection->start();
	else
		std::cout << "Error connection: " << error.message();

	start_accept();
}
