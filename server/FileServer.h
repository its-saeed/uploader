#ifndef FILE_SERVER_H_
#define FILE_SERVER_H_

#include "DownloadWorker.h"
#include <boost/bind.hpp>

class FileServer
{
public:
    FileServer(boost::asio::io_service& io_service)
		:acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 12345))
    {
        start_accept();
    }

private:
    void start_accept()
    {
		DownloadWorker::pointer new_connection =
			DownloadWorker::create(acceptor.get_io_service());

        acceptor.async_accept(new_connection->socket(), 
				boost::bind(&FileServer::handle_accept, this, new_connection,
                    boost::asio::placeholders::error));
    }

	void handle_accept(DownloadWorker::pointer new_connection,
            const boost::system::error_code& error)
    {
        if (!error)
            new_connection->start();
        else
            std::cout << "Error connection: " << error.message();

        start_accept();
    }

	boost::asio::ip::tcp::acceptor acceptor;
};

#endif
