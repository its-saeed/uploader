#include <ctime>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>    
#include <boost/array.hpp>
#include <fstream>

using boost::asio::ip::tcp;
using namespace std;

class tcp_connection : public boost::enable_shared_from_this<tcp_connection>
{
public:
    typedef boost::shared_ptr<tcp_connection> pointer;

    static pointer create(boost::asio::io_service& io_service)
    {
        return pointer(new tcp_connection(io_service));
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        output_stream->open("part_" + std::to_string(std::rand()));
        boost::asio::async_read(socket_, boost::asio::buffer(buffer_),
                boost::bind(&tcp_connection::handle_read, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
    }

private:
    tcp_connection(boost::asio::io_service& io_service)
    :socket_(io_service)
    {
        output_stream = new ofstream;
    }

    void handle_read(const boost::system::error_code& error,
        std::size_t bytes_transferred)
    {
        cout << "bt: " << bytes_transferred << endl;
        if (error)
        {
            socket_.close();
            return;
        }

        if (bytes_transferred)
        {
            output_stream->write(buffer_.data(), buffer_.size());
        }

        boost::asio::async_read(socket_, boost::asio::buffer(buffer_),
                boost::bind(&tcp_connection::handle_read, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
    }

    tcp::socket socket_;
    std::string message_;
    ofstream* output_stream;
    boost::array<char, 1024> buffer_;

};

class tcp_server
{
public:
    tcp_server(boost::asio::io_service& io_service)
        :acceptor(io_service, tcp::endpoint(tcp::v4(), 12345))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        tcp_connection::pointer new_connection = 
            tcp_connection::create(acceptor.get_io_service());

        acceptor.async_accept(new_connection->socket(), 
                boost::bind(&tcp_server::handle_accept, this, new_connection,
                    boost::asio::placeholders::error));
    }

    void handle_accept(tcp_connection::pointer new_connection, 
            const boost::system::error_code& error)
    {
        if (!error)
            new_connection->start();

        start_accept();
    }

    tcp::acceptor acceptor;
};

int main(int argc, char** argv)
{
    boost::asio::io_service io_service;
    tcp_server server(io_service);
    io_service.run();
    return 0;
}
