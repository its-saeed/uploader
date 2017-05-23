#include "UploadWorker.h"
#include <concurrentqueue.h>
#include <boost/algorithm/string.hpp>

extern moodycamel::ConcurrentQueue<std::string> file_parts_queue;

UploadWorker::UploadWorker(boost::asio::io_service& io_service)
: io_service(io_service)
, socket_(io_service)
, send_start_index(0)
, send_end_index(0)
{
    file_stream = new std::ifstream;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12345);
    socket_.async_connect(endpoint, boost::bind(
                &UploadWorker::handle_connect, this, 
                boost::asio::placeholders::error));
}

void UploadWorker::handle_connect(const boost::system::error_code& error)
{
    if (!error)
    {
        std::string item;
        bool en = file_parts_queue.try_dequeue(item);
        parse_file_parts(item);
        if (en)
        {
            file_stream->open(current_file);
            send_bytes_written = 0;
            file_stream->seekg(send_start_index);
            write_file_part();
        }
        else 
            printf("hooy");
    }
}

void UploadWorker::handle_write(const boost::system::error_code& error,
        std::size_t bytes_transferred)
{
    send_bytes_written += bytes_transferred;
    write_file_part();
}

void UploadWorker::parse_file_parts(const std::string& item)
{
    std::vector<std::string> parts;
    split(parts, item, boost::is_any_of("/"));
    current_file = parts.at(0);
    send_start_index = stoi(parts.at(1));
    send_end_index = stoi(parts.at(2));
}

void UploadWorker::write_file_part()
{
    if (send_start_index + send_bytes_written >= send_end_index)
        return;

    char file_content[1024];
    file_stream->read(file_content, 1024);
    boost::asio::async_write(socket_, boost::asio::buffer(file_content, 1024),
            boost::bind(&UploadWorker::handle_write, this, 
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
}
