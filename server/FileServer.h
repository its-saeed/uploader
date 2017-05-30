#ifndef FILE_SERVER_H_
#define FILE_SERVER_H_

#include "DownloadWorker.h"
#include <boost/bind.hpp>

class FileServer
{
public:
    FileServer(boost::asio::io_service& io_service)
		:acceptor1(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 12344))
		,acceptor2(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 12345))
	{
		DownloadWorker::pointer new_connection =
			DownloadWorker::create(acceptor1.get_io_service(), 1024 * 1024);

		acceptor1.async_accept(new_connection->socket(),
				boost::bind(&FileServer::handle_accept, this, new_connection,
					boost::asio::placeholders::error));
		start_accept();
    }

private:
    void start_accept()
    {
		DownloadWorker::pointer new_connection =
			DownloadWorker::create(acceptor2.get_io_service(), 1024 * 1024);

		acceptor2.async_accept(new_connection->socket(),
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

	boost::asio::ip::tcp::acceptor acceptor1;
	boost::asio::ip::tcp::acceptor acceptor2;
};

#endif
